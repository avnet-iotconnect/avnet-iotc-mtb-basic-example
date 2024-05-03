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
//#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"
#include "lwip/apps/sntp.h"

#include "iotconnect.h"
#include "iotc_mtb_time.h"

#include "app_config.h"
#include "app_task.h"
#include "iotc_ota.h"

#define APP_VERSION "02.00.00"
static bool is_demo_mode = false;

#ifdef OTA_SUPPORT
static bool otaflag = false;
#endif



static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            printf("IoTConnect Client Connected notification.\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            printf("IoTConnect Client Disconnected notification.\n");
            break;
        default:
            printf("IoTConnect Client ERROR notification\n");
            break;
    }
}

/******************************************************************************
 * Function Name: wifi_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates connection to the Wi-Fi Access Point using the
 *  specified SSID and PASSWORD. The connection is retried a maximum of
 *  'MAX_WIFI_CONN_RETRIES' times with interval of 'WIFI_CONN_RETRY_INTERVAL_MS'
 *  milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful Wi-Fi connection, else an
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t wifi_connect(void) {
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;
    const char* wifi_ssid = WIFI_SSID;
    const char* wifi_pass = WIFI_PASSWORD;

    /* Check if Wi-Fi connection is already established. */
    if (cy_wcm_is_connected_to_ap() == 0) {
        /* Configure the connection parameters for the Wi-Fi interface. */
        memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
        memcpy(connect_param.ap_credentials.SSID, wifi_ssid, strlen(wifi_ssid));
        memcpy(connect_param.ap_credentials.password, wifi_pass, strlen(wifi_pass));
        connect_param.ap_credentials.security = WIFI_SECURITY;

        printf("Connecting to Wi-Fi AP '%s'\n", connect_param.ap_credentials.SSID);

        /* Connect to the Wi-Fi AP. */
        for (uint32_t retry_count = 0; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++) {
            result = cy_wcm_connect_ap(&connect_param, &ip_address);

            if (result == CY_RSLT_SUCCESS) {
                printf("\nSuccessfully connected to Wi-Fi network '%s'.\n", connect_param.ap_credentials.SSID);

                /* Set the appropriate bit in the status_flag to denote
                 * successful Wi-Fi connection, print the assigned IP address.
                 */
                if (ip_address.version == CY_WCM_IP_VER_V4) {
                    printf("IPv4 Address Assigned: %s\n", ip4addr_ntoa((const ip4_addr_t*) &ip_address.ip.v4));
                } else if (ip_address.version == CY_WCM_IP_VER_V6) {
                    printf("IPv6 Address Assigned: %s\n", ip6addr_ntoa((const ip6_addr_t*) &ip_address.ip.v6));
                }
                return result;
            }

            printf("Connection to Wi-Fi network failed with error code 0x%0X. Retrying in %d ms. Retries left: %d\n",
                    (int) result, WIFI_CONN_RETRY_INTERVAL_MS, (int) (MAX_WIFI_CONN_RETRIES - retry_count - 1));
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
        }

        printf("\nExceeded maximum Wi-Fi connection attempts!\n");
        printf("Wi-Fi connection failed after retrying for %d mins\n",
                (int) ((WIFI_CONN_RETRY_INTERVAL_MS * MAX_WIFI_CONN_RETRIES) / 60000u));
    }
    return result;
}

static void on_ota(IotclC2dEventData data) {
    const char *url = iotcl_c2d_get_ota_url(data, 0);
    if (url == NULL){
    	printf("Download URL is invalid.\r\n");
    	return;
    }
    // printf("Download URL is: %s\n", url);

    const char *otahost = iotcl_c2d_get_ota_url_hostname(data, 0);
    if (otahost == NULL){
    	printf("OTA host is invalid.\r\n");
    	return;
    }
    const char *otapath = iotcl_c2d_get_ota_url_resource(data, 0);
    if (otapath == NULL){
    	printf("OTA resource is invalid.\r\n");
    	return;
    }

    printf("\nOTA host is %s.\nOTA resource is %s.\n", otahost, otapath);

#ifdef OTA_SUPPORT
        /* Start the OTA task */
        if(iotc_ota_start(otahost, otapath, NULL)){
        	printf("OTA starts successfully.\r\n");
        	otaflag = true;
        }
        else {
        	printf("OTA starts unsuccessfully.\r\n");
        	otaflag = false;
        }
#endif
}

// returns success on matching the expected format. Returns is_on, assuming "on" for true, "off" for false
static bool parse_on_off_command(const char* command, const char* name, bool *arg_parsing_success, bool *is_on, const char** message) {
	*arg_parsing_success = false;
	*message = NULL;
	size_t name_len = strlen(name);
    if (0 == strncmp(command, name, name_len)) {
    	if (strlen(command) < name_len + 2) { // one for space and at least one character for the argument
    		printf("ERROR: Expected command \"%s\" to have an argument\n", command);
    		*message = "Command requires an argument";
    		*arg_parsing_success = false;
    	} else if (0 == strcmp(&command[name_len + 1], "on")) {
    		*is_on = true;
    		*message = "Value is now \"on\"";
    		*arg_parsing_success = true;
    	} else if (0 == strcmp(&command[name_len + 1], "off")) {
    		*is_on = false;
    		*message = "Value is now \"off\"";
    		*arg_parsing_success = true;
    	} else {
    		*message = "Command argument";
    		*arg_parsing_success = false;
    	}
    	// we matches the command
		return true;
    }

    // not our command
	return false;
}

static void on_command(IotclC2dEventData data) {
	const char * const BOARD_STATUS_LED = "board-user-led";
	const char * const DEMO_MODE_CMD = "demo-mode";
    bool command_success = false;
    const char * message = NULL;

    const char *command = iotcl_c2d_get_command(data);
    const char *ack_id = iotcl_c2d_get_ack_id(data);
    if (ack_id) {
    	printf("WARNING: Acknowledgments are not supported with this software version!");
    	ack_id = NULL; // Force code flow to be the same as if there was no ACK.
    }
    if (command) {
    	bool arg_parsing_success;
        printf("Command %s received with %s ACK ID\n", command, ack_id ? ack_id : "no");
        // could be a command without acknowledgement, so ackID can be null
        bool led_on;
        if (parse_on_off_command(command, BOARD_STATUS_LED, &arg_parsing_success, &led_on, &message)) {
        	command_success = arg_parsing_success;
        	if (arg_parsing_success) {
                cyhal_gpio_write(CYBSP_USER_LED, !led_on); // USER_LED is active low
        	}
        } else if (parse_on_off_command(command, DEMO_MODE_CMD,  &arg_parsing_success, &is_demo_mode, &message)) {
        	command_success = arg_parsing_success;
        } else {
            printf("Failed to parse command\n");
        	message = "Unrecognized command";
        }
    } else {
    	printf("Failed to parse command. Command missing?\n");
        message = "Parsing error";
    }

    // could be a command without ack, so ack ID can be null
    // the user needs to enable acknowledgements in the template to get an ack ID
	if (ack_id) {
		iotcl_mqtt_send_cmd_ack(
				ack_id,
				command_success ? IOTCL_C2D_EVT_CMD_SUCCESS : IOTCL_C2D_EVT_CMD_FAILED,
				message // allowed to be null, but should not be null if failed, we'd hope
		);
	} else {
		// if we send an ack
		printf("Message status is %s. Message: %s\n", command_success ? "SUCCESS" : "FAILED", message ? message : "<none>");
	}
}

static cy_rslt_t publish_telemetry(void) {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "random", random() % 100); // test some random numbers

    iotcl_mqtt_send_telemetry(msg, false);
    iotcl_telemetry_destroy(msg);
    return CY_RSLT_SUCCESS;
}


void app_task(void *pvParameters) {
    (void) pvParameters;
    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    // printf("\x1b[2J\x1b[;H");
    printf("===============================================================\n");
    printf("Starting The App Task\n");
    printf("===============================================================\n\n");

#if defined(OTA_SUPPORT) && defined(OTA_USE_EXTERNAL_FLASH)
    /* We need to init external flash */
	iotc_ota_init();

    /* Validate the update */
	iotc_ota_storage_validated();
#endif /* OTA_USE_EXTERNAL_FLASH */

    uint64_t hwuid = Cy_SysLib_GetUniqueId();
    uint32_t hwuidhi = (uint32_t)(hwuid >> 32);
    // not using low bytes in the name because they appear to be the same across all boards of the same type
    // feel free to modify the application to use these bytes
    //uint32_t hwuidlo = (uint32_t)(hwuid & 0xFFFFFFFF);

    char iotc_duid[IOTCL_CONFIG_DUID_MAX_LEN];
	sprintf(iotc_duid, IOTCONNECT_DUID_PREFIX"%08lx", hwuidhi);

    printf("Generated device unique ID (DUID) is: %s\n", iotc_duid);

    IotConnectClientConfig config;
    iotconnect_sdk_init_config(&config);
    config.connection_type = IOTCONNECT_CONNECTION_TYPE;
    config.cpid = IOTCONNECT_CPID;
    config.env =  IOTCONNECT_ENV;
    config.duid = iotc_duid;
    config.qos = 1;
    config.verbose = true;
    config.x509_config.device_cert = IOTCONNECT_DEVICE_CERT;
    config.x509_config.device_key = IOTCONNECT_DEVICE_KEY;
    config.callbacks.status_cb = on_connection_status;
    config.callbacks.cmd_cb = on_command;
    config.callbacks.ota_cb = on_ota;

    const char * conn_type_str = "(UNKNOWN)";
    if (config.connection_type == IOTC_CT_AWS) {
    	conn_type_str = "AWS";
    } else if(config.connection_type == IOTC_CT_AZURE) {
    	conn_type_str = "Azure";
    }

    printf("Current Settings:\n");
    printf("Platform: %s\n", conn_type_str);
    printf("DUID: %s\n", config.duid);
    printf("CPID: %s\n", config.cpid);
    printf("ENV: %s\n", config.env);
    printf("WiFi SSID: %s\n", WIFI_SSID);

    cy_wcm_config_t wcm_config = { .interface = CY_WCM_INTERFACE_TYPE_STA };
    if (CY_RSLT_SUCCESS != cy_wcm_init(&wcm_config)) {
        printf("Error: Wi-Fi Connection Manager initialization failed!\n");
        goto exit_cleanup;
    }

    printf("Wi-Fi Connection Manager initialized.\n");

    /* Initiate connection to the Wi-Fi AP and cleanup if the operation fails. */
    if (CY_RSLT_SUCCESS != wifi_connect()) {
        goto exit_cleanup;
    }

    if (0 != iotc_mtb_time_obtain(IOTCONNECT_SNTP_SERVER)) {
        // called function will print errors
        return;
    }

    cy_rslt_t ret = iotconnect_sdk_init(&config);
    if (CY_RSLT_SUCCESS != ret) {
        printf("Failed to initialize the IoTConnect SDK. Error code: %lu\n", ret);
        goto exit_cleanup;
    }

    cyhal_gpio_write(CYBSP_USER_LED, false); // USER_LED is active low

    for (int i = 0; i < 10; i++) {
    	ret = iotconnect_sdk_connect();
        if (CY_RSLT_SUCCESS != ret) {
            printf("Failed to initialize the IoTConnect SDK. Error code: %lu\n", ret);
            goto exit_cleanup;
        }

        int max_messages = is_demo_mode ? 600 : 30; // non-demo = 5 seconds * 10 * 30 = 25 minutes ; demo = 5 seconds * 10 * 600 = 8 hours
        for (int j = 0; iotconnect_sdk_is_connected() && j < max_messages; j++) {
#ifdef OTA_SUPPORT
        	if (otaflag == true) {
                break;
        	}
#endif
        	cy_rslt_t result = publish_telemetry();
        	if (result != CY_RSLT_SUCCESS) {
        		break;
        	}
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
        iotconnect_sdk_disconnect();
    }
    iotconnect_sdk_deinit();

	printf("\nAppTask Done.\nTerminating the AppTask...\n");
	while (1) {
		taskYIELD();
	}
    return;

    exit_cleanup:
	printf("\nError encountered. AppTask Done.\nTerminating the AppTask...\n");
	while (1) {
		taskYIELD();
	}

}
