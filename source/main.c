/******************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for MQTT Client Example for ModusToolbox.
 *
 * Related Document: See README.md
 *
 *
 *******************************************************************************
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

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cy_log.h"

#include "app_task.h"

/******************************************************************************
 * Global Variables
 ******************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, sets up 
 *  the MQTT client task, and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/

// A VERY crude implementation of log output so we can at least see some messages
int app_log_output_callback(CY_LOG_FACILITY_T facility, CY_LOG_LEVEL_T level, char *logmsg) {
  (void)facility;     // Can be used to decide to reduce output or send output to remote logging
  (void)level;        // Can be used to decide to reduce output, although the output has already been
                      // limited by the log routines

  return printf( "%s", logmsg);   // print directly to console
}

// A VERY crude implementation for obtaining timestamp (always 0) for logs so we can at least see some messages
cy_rslt_t app_log_time(uint32_t* time) {
    if (time != NULL) {
        *time = 0;
    }
    return CY_RSLT_SUCCESS;
}

int main() {
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE) || defined (IOTC_OTA_SUPPORT)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif /* #if defined (CY_DEVICE_SECURE) || defined (IOTC_OTA_SUPPORT) */

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* default for all logging to WARNING */
    cy_log_init(CY_LOG_WARNING, app_log_output_callback, app_log_time);

    // Use CY_LOG_INFO or debug here to get more info from HTTP and similar
    // if encountering issues, but OTA on AWS will likely encounter issues
    // since it appears to be timing sensitive
    CY_LOG_LEVEL_T mw_log_level = CY_LOG_WARNING;
    cy_log_set_facility_level(CYLF_MIDDLEWARE, mw_log_level);

#if defined (IOTC_OTA_SUPPORT)
    // The OTA v4.2.0 agent has a bug where it overrides the value of middleware log level to CY_LOG_ERR
    // This is the workaround for the bug when we need to something other than CY_LOG_ERR.
    extern CY_LOG_LEVEL_T ota_logging_level;
    ota_logging_level = mw_log_level;
#endif

    /* Create the MQTT Client task. */
    xTaskCreate(app_task, "App Task", APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */
