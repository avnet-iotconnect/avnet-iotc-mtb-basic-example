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
#include "iotc_ota.h"


#define APP_VERSION "01.00.00"

/* MAX connection retries to join WI-FI AP */
#define MAX_CONNECTION_RETRIES              (10u)

/* Wait between connection retries */
#define WIFI_CONN_RETRY_DELAY_MS            (500)
/*******************************************************************************
* Forward declaration
********************************************************************************/
static cy_rslt_t publish_telemetry(void);
static void on_ota(IotclEventData data);
cy_rslt_t connect_to_wifi_ap(void);
static bool spliturl(const char *url, char **host_name, char**resource);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* OTA context */
volatile static bool otaFlag = false;


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



void app_task(void *pvParameters) {

#if defined(OTA_SUPPORT) && defined(OTA_USE_EXTERNAL_FLASH)
    /* We need to init external flash */
	iotc_ota_init();

    /* Validate the update */
	iotc_ota_storage_validated();
#endif /* OTA_USE_EXTERNAL_FLASH */

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
        iotc_config->ota_cb = on_ota;

        cy_rslt_t ret = iotconnect_sdk_init();
        if (CY_RSLT_SUCCESS != ret) {
            printf("Failed to initialize the IoTConnect SDK. Error code: %lu\n", ret);
            goto exit_cleanup;
        }

        for (int i = 0; iotconnect_sdk_is_connected() && i < 10; i++)
        {
        	if(otaFlag == true){
                goto exit_cleanup;
        	}
            cy_rslt_t result = publish_telemetry();
        	if (result != CY_RSLT_SUCCESS) {
        		break;
        	}
        	vTaskDelay(pdMS_TO_TICKS(10000));
        }
        iotconnect_sdk_disconnect();
    }
    exit_cleanup: printf("\nAppTask Done.\nTerminating the AppTask...\n");
    vTaskDelete(NULL);

}

static cy_rslt_t publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    cy_rslt_t result = iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
	return result;
}


static void on_ota(IotclEventData data)
{
	char *otahost;
	char *otapath;
	char **host = &otahost;
	char **path = &otapath;

    char *url = iotcl_clone_download_url(data, 0);
    if (url == NULL){
    	printf("Download URL is invalid.\r\n");
    	return;
    }

    bool status = spliturl(url, host, path);
    if (!status) {
        printf("start_ota: Error while splitting the URL, code: 0x%x\r\n", status);
    }
    else {
    	printf("\nHOST is %s.\n\nPATH is %s.\r\n", otahost, otapath);

#ifdef OTA_SUPPORT
        /* Start the OTA task */
        if(iotc_ota_start(otahost, otapath, NULL)){
        	printf("OTA starts successfully.\r\n");
        	otaFlag = true;
        }
        else {
        	printf("OTA starts unsuccessfully.\r\n");
        	otaFlag = false;
        }
#endif
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
