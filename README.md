## About
Avnet IoTConnect basic example for Infineon's ModusToolbox IDE and framework.

## Dependencies
The project uses the following dependent projects:
* [avnet-iotc-mtb-sdk](https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk)

## Supported Toolchains (make variable 'TOOLCHAIN')
> [!IMPORTANT]
> As of 8/8/2025, the older version v7.0.2 of this application will not work with any MTB Tools versions 
> Due to a problem with incompatibility with retarget-io and the latest BSP.
> Please use v7.1.0 and later versions.

* GNU Arm® Embedded Compiler (GCC_ARM) - Default value of TOOLCHAIN

## Supported Boards
The code has been developed and tested with MTB 3.3, with VsCode, and the boards below:
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CY8CKIT-062-WiFi-BT) (`CY8CKIT-062-WIFI-BT`)
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; prototyping kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/) (`CY8CPROTO-062-4343W`)
- [PSoC&trade; 62S2 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/) (`CY8CKIT-062S2-43012`)
- [PSoC&trade; 6 CY8CKIT-062S2-AI](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-ai/)(`CY8CKIT-062S2-AI`)

The following boards *should* work without any code modification, but have not been tested:
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cyw9p62s1-43438evb-01/) (`CYW9P62S1-43438EVB-01`) - Product EOL
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cyw9p62s1-43012evb-01/) (`CYW9P62S1-43012EVB-01`)
- [PSoC&trade; 62S2 evaluation kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/) (`CY8CEVAL-062S2-LAI-4373M2`, `CY8CEVAL-062S2-MUR-43439M2`)
- [PSoC&trade; 62S3 Wi-Fi Bluetooth&reg; prototyping kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062s3-4343w/) (`CY8CPROTO-062S3-4343W`)

## Instructions
* Please refer to the [DEVELOPER_GUIDE](https://github.com/avnet-iotconnect/iotc-modustoolbox-example/blob/main/DEVELOPER_GUIDE.md).
