Avnet IoTConnect project example for Infineon's ModusToolbox framework. This code is based on the [mtb-example-anycloud-mqtt-client](https://github.com/Infineon/mtb-example-anycloud-mqtt-client) project and indirectly on the Infineon's [MQTT 3.1.1](https://github.com/Infineon/mqtt) client library.

## Dependencies
The project uses the following dependent projects:
* [iotc-modustoolbox-sdk](https://github.com/avnet-iotconnect/iotc-modustoolbox-sdk)

## Supported Boards
The code has been developed and tested With:
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; pioneer kit](https://www.cypress.com/CY8CKIT-062-WiFi-BT) (`CY8CKIT-062-WIFI-BT`)

The following boards *should* work without any code modification, but have not been tested:
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; prototyping kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/) (`CY8CPROTO-062-4343W`)
- [PSoC&trade; 62S2 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/) (`CY8CKIT-062S2-43012`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cyw9p62s1-43438evb-01/) (`CYW9P62S1-43438EVB-01`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cyw9p62s1-43012evb-01/) (`CYW9P62S1-43012EVB-01`)
- [PSoC&trade; 62S2 evaluation kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/) (`CY8CEVAL-062S2-LAI-4373M2`, `CY8CEVAL-062S2-MUR-43439M2`)
- [PSoC&trade; 62S3 Wi-Fi Bluetooth&reg; prototyping kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062s3-4343w/) (`CY8CPROTO-062S3-4343W`)

## Build Instructions

- Download, install and open [ModusToolbox&trade; software](https://www.cypress.com/products/modustoolbox-software-environment) v3.0.0 or later (tested with v3.0.0). On Windows, ensure that you **Run As Adminstrator** the installation package so that the neccessary drivers can be installed.
- Select a name for your workspace when prompted for a workspace name.
- Click the **New Application** link in the **Quick Panel** (or, use **File** > **New** > **ModusToolbox Application**). This launches the [Project Creator](https://www.cypress.com/ModusToolboxProjectCreator) tool.
- Pick a kit supported by the code example from the list shown in the **Project Creator - Choose Board Support Package (BSP)** dialog and click **Next**
- In the **Project Creator - Select Application** dialog, Click the checkbox of the project **Avnet IoTConnect Basic Example** under **Wi-Fi** catergory and then click **Create**. 
- Modify **Avnet_IoTConnect_Basic_Example/configs/app_config.h** per your IoTConnect device and account info.
- Modify **Avnet_IoTConnect_Basic_Example/configs/wifi_config.h** with your WIFI_SSID and WIFI_PASSWORD.
- At this point you should be able to build and run the application by using the options in the **Quick Panel** on bottom left of the screen.   
- You should see the application output in your terminal emulator.

NOTE: If you cloned the repo, note that the SDK will be pulled from this GitHub URL instead of being used from the local repo, and modifying the contents of iotc-modustoolbox-sdk will have no effect on your code execution. 
If you need to make temporary modifications to the SDK, you can modify the contents in the mtb_shared directory.

