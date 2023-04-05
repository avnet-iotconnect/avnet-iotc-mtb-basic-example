/*******************************************************************************
* Copyright 2020-2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
//
// Copyright: Avnet 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//

#include "cyhal.h"
#include "cybsp.h"

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Configuration file for Wi-Fi and MQTT client */
#include "wifi_config.h"

/* Middleware libraries */
#include "cy_retarget_io.h"
//#include "cy_wcm.h"
//#include "cy_lwip.h"

//#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"
#include "lwip/apps/sntp.h"

#include "iotconnect.h"
#include "iotconnect_common.h"
#include "iotc_mtb_time.h"

#include "app_config.h"
#include "app_task.h"


#include "cy_log.h"

/* OTA API */
#include "cy_ota_api.h"
#include "ota_serial_flash.h"

/* App specific configuration */
#include "ota_app_config.h"

#define APP_VERSION "01.00.00"

/* MAX connection retries to join WI-FI AP */
#define MAX_CONNECTION_RETRIES              (10u)

/* Wait between connection retries */
#define WIFI_CONN_RETRY_DELAY_MS            (500)
/*******************************************************************************
* Forward declaration
********************************************************************************/

cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data);
void print_heap_usage(char *msg);


/*******************************************************************************
* Global Variables
********************************************************************************/
/* OTA context */
cy_ota_context_ptr ota_context;
volatile static bool otaFlag = false;

/* Network parameters for OTA */
cy_ota_network_params_t ota_network_params =
{
    .http =
    {
        .server =
        {
            .host_name = HTTP_SERVER,
            .port = HTTP_SERVER_PORT
        },
        .file = OTA_HTTP_JOB_FILE,
    #if (ENABLE_TLS == true)
        .credentials =
        {
            .root_ca = ROOT_CA_CERTIFICATE,
            .root_ca_size = sizeof(ROOT_CA_CERTIFICATE),
        #if (USING_CLIENT_CERTIFICATE == true)
            .client_cert = CLIENT_CERTIFICATE,
            .client_cert_size = sizeof(CLIENT_CERTIFICATE),
        #endif
        #if (USING_CLIENT_KEY == true)
            .private_key = CLIENT_KEY,
            .private_key_size = sizeof(CLIENT_KEY),
        #endif
        },
    #endif
    },
    .use_get_job_flow = CY_OTA_DIRECT_FLOW,
#if (ENABLE_TLS == true)
    .initial_connection = CY_OTA_CONNECTION_HTTPS,
#else
    .initial_connection = CY_OTA_CONNECTION_HTTP
#endif
};

/* Parameters for OTA agent */
cy_ota_agent_params_t ota_agent_params =
{
    .cb_func = ota_callback,
    .cb_arg = &ota_context,
    .reboot_upon_completion = 1,
    .validate_after_reboot = 1,
    .do_not_send_result = 1
};

/* Macro to check if the result of an operation was successful and set the
 * corresponding bit in the status_flag based on 'init_mask' parameter. When
 * it has failed, print the error message and return the result to the
 * calling function.
 */
#define CHECK_RESULT(result, init_mask, error_message...)      \
                     do                                        \
                     {                                         \
                         if ((int)result == CY_RSLT_SUCCESS)   \
                         {                                     \
                             status_flag |= init_mask;         \
                         }                                     \
                         else                                  \
                         {                                     \
                             printf(error_message);            \
                             return result;                    \
                         }                                     \
                     } while(0)



/*******************************************************************************
 * Function Name: ota_callback()
 *******************************************************************************
 * Summary:
 *  Prints the status of the OTA agent on every event. This callback is optional,
 *  but be aware that the OTA middleware will not print the status of OTA agent
 *  on its own.
 *
 * Return:
 *  CY_OTA_CB_RSLT_OTA_CONTINUE - OTA Agent to continue with function.
 *  CY_OTA_CB_RSLT_OTA_STOP     - OTA Agent to End current update session.
 *  CY_OTA_CB_RSLT_APP_SUCCESS  - Application completed task, success.
 *  CY_OTA_CB_RSLT_APP_FAILED   - Application completed task, failure.
 *
 *******************************************************************************/
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data)
{
    cy_ota_callback_results_t   cb_result = CY_OTA_CB_RSLT_OTA_CONTINUE;
    const char                  *state_string;
    const char                  *error_string;

    if (cb_data == NULL)
    {
        return CY_OTA_CB_RSLT_OTA_STOP;
    }

    state_string  = cy_ota_get_state_string(cb_data->ota_agt_state);
    error_string  = cy_ota_get_error_string(cy_ota_get_last_error());

    print_heap_usage("In OTA Callback");

    switch (cb_data->reason)
    {

        case CY_OTA_LAST_REASON:
            break;

        case CY_OTA_REASON_SUCCESS:
            printf(">> APP CB OTA SUCCESS state:%d %s last_error:%s\n\n",
                    cb_data->ota_agt_state,
                    state_string, error_string);
            break;

        case CY_OTA_REASON_FAILURE:
            printf(">> APP CB OTA FAILURE state:%d %s last_error:%s\n\n",
                    cb_data->ota_agt_state, state_string, error_string);
            break;

        case CY_OTA_REASON_STATE_CHANGE:
            switch (cb_data->ota_agt_state)
            {
                case CY_OTA_STATE_NOT_INITIALIZED:
                case CY_OTA_STATE_EXITING:
                case CY_OTA_STATE_INITIALIZING:
                case CY_OTA_STATE_AGENT_STARTED:
                case CY_OTA_STATE_AGENT_WAITING:
                    break;

                case CY_OTA_STATE_START_UPDATE:
                    printf("APP CB OTA STATE CHANGE CY_OTA_STATE_START_UPDATE\n");
                    break;

                case CY_OTA_STATE_JOB_CONNECT:
                    printf("APP CB OTA CONNECT FOR JOB using ");
                    /* NOTE:
                     *  HTTP - json_doc holds the HTTP "GET" request
                     */
                    if ((cb_data->broker_server.host_name == NULL) ||
                        ( cb_data->broker_server.port == 0) ||
                        ( strlen(cb_data->file) == 0) )
                    {
                        printf("ERROR in callback data: HTTP: server: %p port: %d topic: '%p'\n",
                                cb_data->broker_server.host_name,
                                cb_data->broker_server.port,
                                cb_data->file);
                        cb_result = CY_OTA_CB_RSLT_OTA_STOP;
                    }
                    printf("HTTP: server:%s port: %d file: '%s'\n",
                            cb_data->broker_server.host_name,
                            cb_data->broker_server.port,
                            cb_data->file);

                    break;

                case CY_OTA_STATE_JOB_DOWNLOAD:
                    printf("APP CB OTA JOB DOWNLOAD using ");
                    /* NOTE:
                     *  HTTP - json_doc holds the HTTP "GET" request
                     */
                    printf("HTTP: '%s'\n", cb_data->file);
                    break;

                case CY_OTA_STATE_JOB_DISCONNECT:
                    printf("APP CB OTA JOB DISCONNECT\n");
                    break;

                case CY_OTA_STATE_JOB_PARSE:
                    printf("APP CB OTA PARSE JOB: '%.*s' \n",
                    strlen(cb_data->json_doc),
                    cb_data->json_doc);
                    break;

                case CY_OTA_STATE_JOB_REDIRECT:
                    printf("APP CB OTA JOB REDIRECT\n");
                    break;

                case CY_OTA_STATE_DATA_CONNECT:
                    printf("APP CB OTA CONNECT FOR DATA using ");
                    printf("HTTP: %s:%d \n", cb_data->broker_server.host_name,
                    cb_data->broker_server.port);
                    break;

                case CY_OTA_STATE_DATA_DOWNLOAD:
                    printf("APP CB OTA DATA DOWNLOAD using ");
                    /* NOTE:
                     *  HTTP - json_doc holds the HTTP "GET" request
                     */
                    printf("HTTP: '%.*s' ", strlen(cb_data->json_doc), cb_data->json_doc);
                    printf("File: '%s'\n\n", cb_data->file);
                    break;

                case CY_OTA_STATE_DATA_DISCONNECT:
                    printf("APP CB OTA DATA DISCONNECT\n");
                    break;

                case CY_OTA_STATE_RESULT_CONNECT:
                    printf("APP CB OTA SEND RESULT CONNECT using ");
                    /* NOTE:
                     *  HTTP - json_doc holds the HTTP "GET" request
                     */
                    printf("HTTP: Server:%s port: %d\n",
                            cb_data->broker_server.host_name,
                            cb_data->broker_server.port);
                    break;

                case CY_OTA_STATE_RESULT_SEND:
                    printf("APP CB OTA SENDING RESULT using ");
                    /* NOTE:
                     *  HTTP - json_doc holds the HTTP "PUT"
                     */
                    printf("HTTP: '%s' \n", cb_data->json_doc);
                    break;

                case CY_OTA_STATE_RESULT_RESPONSE:
                    printf("APP CB OTA Got Result response\n");
                    break;

                case CY_OTA_STATE_RESULT_DISCONNECT:
                    printf("APP CB OTA Result Disconnect\n");
                    break;

                case CY_OTA_STATE_OTA_COMPLETE:
                    printf("APP CB OTA Session Complete\n");
                    break;

                case CY_OTA_STATE_STORAGE_OPEN:
                    printf("APP CB OTA STORAGE OPEN\n");
                    break;

                case CY_OTA_STATE_STORAGE_WRITE:
                    printf("APP CB OTA STORAGE WRITE %ld%% (%ld of %ld)\n",
                            (unsigned long)cb_data->percentage,
                            (unsigned long)cb_data->bytes_written,
                            (unsigned long)cb_data->total_size);

                    /* Move cursor to previous line */
                    printf("\x1b[1F");
                    break;

                case CY_OTA_STATE_STORAGE_CLOSE:
                    printf("APP CB OTA STORAGE CLOSE\n");
                    break;

                case CY_OTA_STATE_VERIFY:
                    printf("APP CB OTA VERIFY\n");
                    break;

                case CY_OTA_STATE_RESULT_REDIRECT:
                    printf("APP CB OTA RESULT REDIRECT\n");
                    break;

                case CY_OTA_NUM_STATES:
                    break;
            }   /* switch state */
            break;
    }

    return cb_result;
}


static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}

static bool spliturl(const char *url, char **host_name, char**resource) {
    int host_name_start = 0;
    size_t url_len = strlen(url);

    if (!host_name || !resource) {
        printf("split_url: Invalid usage\r\n");
        return false;
    }

    *host_name = NULL;
    *resource = NULL;
    int slash_count = 0;
    for (size_t i = 0; i < url_len; i++) {
        if (url[i] == '/') {
            slash_count++;
            if (slash_count == 2) {
                host_name_start = i + 1;
            } else if (slash_count == 3) {
                const size_t slash_start = i;
                const size_t host_name_len = i - host_name_start;
                const size_t resource_len = url_len - i;
                *host_name = malloc(host_name_len + 1); //+1 for null
                if (NULL == *host_name) {
                    return false;
                }
                memcpy(*host_name, &url[host_name_start], host_name_len);
                (*host_name)[host_name_len] = 0; // terminate the string

                *resource = malloc(resource_len + 1); //+1 for null
                if (NULL == *resource) {
                    free(*host_name);
                    return false;
                }
                memcpy(*resource, &url[slash_start], resource_len);
                (*resource)[resource_len] = 0; // terminate the string

                return true;
            }
        }
    }
    return false; // URL could not be parsed
}



void start_ota(IotclEventData data)
{
	otaFlag = true;

	char *otahost;
	char *otapath;
	char **host = &otahost;
	char **path = &otapath;

    char *url = iotcl_clone_download_url(data, 0);
//    printf("\r\nURL is %s\r\n", url);

    bool status = spliturl(url, host, path);
    if (!status) {
        printf("start_ota: Error while splitting the URL, code: 0x%x\r\n", status);
    }
    printf("\r\nHOST is %s\r\nPATH is %s\r\n", otahost, otapath);

    ota_network_params.http.file = otapath;
    printf("\r\nHTTP FILE is %s\r\n", ota_network_params.http.file);

//    free(host);
//    free(path);
    if( cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_context) != CY_RSLT_SUCCESS )
    {
        printf("\n Initializing and starting the OTA agent failed.\n");
        CY_ASSERT(0);
    }
}

cy_rslt_t connect_to_wifi_ap(void)
{
    cy_wcm_config_t wifi_config = { .interface = CY_WCM_INTERFACE_TYPE_STA};
    cy_wcm_connect_params_t wifi_conn_param;
    cy_wcm_ip_address_t ip_address;
    cy_rslt_t result;

    /* Variable to track the number of connection retries to the Wi-Fi AP specified
     * by WIFI_SSID macro. */
    uint32_t conn_retries = 0;

    /* Initialize Wi-Fi connection manager. */
    cy_wcm_init(&wifi_config);

     /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY;

    /* Connect to the Wi-Fi AP */
    for(conn_retries = 0; conn_retries < MAX_CONNECTION_RETRIES; conn_retries++)
    {
        result = cy_wcm_connect_ap( &wifi_conn_param, &ip_address );

        if (result == CY_RSLT_SUCCESS)
        {
            printf( "Successfully connected to Wi-Fi network '%s'.\n",
                    wifi_conn_param.ap_credentials.SSID);
            return result;
        }

        printf( "Connection to Wi-Fi network failed with error code %d."
                "Retrying in %d ms...\n", (int) result, WIFI_CONN_RETRY_DELAY_MS );
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_DELAY_MS));
    }

    printf( "Exceeded maximum Wi-Fi connection attempts\n" );

    return result;
}

void app_task(void *pvParameters) {

    /* default for OTA logging to NOTiCE */
//    cy_ota_set_log_level(CY_LOG_WARNING);

#if defined(OTA_USE_EXTERNAL_FLASH)
    /* We need to init from every ext flash write
     * See ota_serial_flash.h
     */

    /* initialize SMIF interface */
    printf("call ota_smif_initialize()\n");
    if (ota_smif_initialize() != CY_RSLT_SUCCESS)
    {
        printf("ERROR returned from ota_smif_initialize()!!!!!\n");
    }
#endif /* OTA_USE_EXTERNAL_FLASH */

	printf("APPLICATION VERSION is v%d.%d.%d\r\n", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);

    /* Validate the update so we do not revert */
    if(cy_ota_storage_validated() != CY_RSLT_SUCCESS)
    {
        printf("\n Failed to validate the update.\n");
    }

    /* Connect to Wi-Fi AP */
    if( connect_to_wifi_ap() != CY_RSLT_SUCCESS )
    {
        printf("\n Failed to connect to Wi-FI AP.\n");
        CY_ASSERT(0);
    }

    if (0 != iotc_mtb_time_obtain(IOTCONNECT_SNTP_SERVER)) {
        // called function will print errors
        return;
    }

    for (int i = 0; i < 100; i++) {

        IotConnectClientConfig *iotc_config = iotconnect_sdk_init_and_get_config();
        iotc_config->duid = IOTCONNECT_DUID;
        iotc_config->cpid = IOTCONNECT_CPID;
        iotc_config->env =  IOTCONNECT_ENV;
        iotc_config->auth.type = IOTCONNECT_AUTH_TYPE;

        if (iotc_config->auth.type == IOTC_AT_X509) {
            iotc_config->auth.data.cert_info.device_cert = IOTCONNECT_DEVICE_CERT;
            iotc_config->auth.data.cert_info.device_key = IOTCONNECT_DEVICE_KEY;
        }
        iotc_config->ota_cb = start_ota;

        cy_rslt_t ret = iotconnect_sdk_init();
        if (CY_RSLT_SUCCESS != ret) {
            printf("Failed to initialize the IoTConnect SDK. Error code: %lu\n", ret);
            goto exit_cleanup;
        }

        for (int i = 0; iotconnect_sdk_is_connected() && i < 100; i++)
        {
        	if(otaFlag == true){
                goto exit_cleanup;
        	}
        	publish_telemetry();
        	vTaskDelay(pdMS_TO_TICKS(10000));
        }
        iotconnect_sdk_disconnect();
    }
    exit_cleanup: printf("\nAppTask Done.\nTerminating the AppTask...\n");
    vTaskDelete(NULL);

}

