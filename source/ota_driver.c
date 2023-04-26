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
// Modified by Shu Liu <shu.liu@avnet.com> on 04/25/23.
//

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Middleware libraries */
#include "cy_retarget_io.h"
#include "cy_log.h"

/* OTA API */
#include "cy_ota_api.h"
#include "ota_serial_flash.h"
#include "app_config.h"

cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data);
void print_heap_usage(char *msg);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* OTA context */
static cy_ota_context_ptr ota_context;

/* Network parameters for OTA */
static cy_ota_network_params_t ota_network_params =
{
    .http =
    {
        .server =
        {
            .host_name = NULL,
            .port = HTTP_SERVER_PORT
        },
        .file = OTA_HTTP_JOB_FILE,

        .credentials =
        {
            .root_ca = ROOT_CA_CERTIFICATE,
            .root_ca_size = sizeof(ROOT_CA_CERTIFICATE),
        },
    },
    .use_get_job_flow = CY_OTA_DIRECT_FLOW,
    .initial_connection = CY_OTA_CONNECTION_HTTPS,
};

/* Parameters for OTA agent */
static cy_ota_agent_params_t ota_agent_params =
{
    .cb_func = ota_callback,
    .cb_arg = &ota_context,
    .reboot_upon_completion = 1,
    .validate_after_reboot = 1,
    .do_not_send_result = 1
};


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


bool spliturl(const char *url, char **host_name, char**resource) {
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

bool start_ota(char* host, char* path)
{
   ota_network_params.http.file = path;
   ota_network_params.http.server.host_name = host;
   printf("\r\nHTTP FILE is %s\r\n", ota_network_params.http.file);

  if( cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_context) != CY_RSLT_SUCCESS )
  {
    printf("\n Initializing and starting the OTA agent failed.\n");
    return false;
  }
  return true;
}




