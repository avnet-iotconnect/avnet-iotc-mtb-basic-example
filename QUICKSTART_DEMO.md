## Introduction

This document demonstrates the steps of setting up the Infineon PSoC&trade; 6 boards
for connecting to Avnet's IoTConnect Platform. Supported boards are listed in the [README](https://github.com/avnet-iotconnect/iotc-modustoolbox-example/blob/main/README.md).

## Hardware Setup

* The board needs to be connected to a USB port on your PC via the Micro USB cable.
* Once the board boots up, a new USB devices will be available. 
  * Optionally, connect a terminal program (like TeraTerm) to one of the two COM ports
which is named "USB Serial". Use defaults for 115200 baud: 8 bits, 1 stop bit, no flow control or parity. 
Firmware logs will be available on that COM port. 


## Software setup
- Download, install and open [ModusToolbox&trade; software](https://www.cypress.com/products/modustoolbox-software-environment) v3.0.0 or later (tested with v3.0.0). On Windows, ensure that you **Run As Adminstrator** the installation package so that the neccessary drivers can be installed.
- Select a name for your workspace when prompted for a workspace name.
- Click the **New Application** link in the **Quick Panel** (or, use **File** > **New** > **ModusToolbox Application**). This launches the [Project Creator](https://www.cypress.com/ModusToolboxProjectCreator) tool.
- Pick a kit supported by the code example from the list shown in the **Project Creator - Choose Board Support Package (BSP)** dialog and click **Next**
- In the **Project Creator - Select Application** dialog, Click the checkbox of the project **Avnet IoTConnect Basic Example** under **Wi-Fi** catergory and then click **Create**. 
- Modify **Avnet_IoTConnect_Basic_Example/configs/app_config.h** per your IoTConnect device and account & security info.
- Modify **Avnet_IoTConnect_Basic_Example/configs/wifi_config.h** with your WIFI_SSID and WIFI_PASSWORD.
- At this point you should be able to build and run the application by using the options in the **Quick Panel** on bottom left of the screen.   
- You should see the application output in your terminal emulator.

## IoTConnect Template Setup

* Login to the IoTConnect platform, navigate to Devices -> Device -> Templates and click "Create Template."
  * Enter a name like "xxxxdemo" for **Template Code** and **Template Name**. 
  * Select *Symmetric Key* or *Self Signed Certificate* in the **Authentication Type** pulldown.
  * Ensure that **Device Message Version** is **1.0**
  * Click **Save**
* On the same page, click the Attributes tab.
* You can add some attributes to the list of attributes, for example:
  * *sensordata*  - "NUMBER"
  * *temperature* - "NUMBER"
  * *button*      - "NUMBER"
  * *version*     - "STRING"
* The screenshot below shows an example template:

<img src="media/template.png" width="50%" height="50%" />


## Setting up the Device in IoTConnect
 
* Navigate to Device -> Device and click the **Create Device** button.
* Your device unique ID is the "snXXXXXXXX" collection numbers and letters by which the files on the MSD are created. 
Enter that name (including "sn") as the device name and display name.
* Choose your entity where you will create the device.
* Select the Template that was created in a previous step.
* Enter the fingerprint of your certificate if you use **Self Signed Certificate** as the Authentication Type.
* Click **Save**.

<img src="media/iotc-device.png" width="50%" height="50%" />


## Appendix

### Obtaining the Device Certificate Fingerprint

This section outlines how to set up the device for IoTConnect Self Signed Certificate authentication type.
Steps for other authentication types are out of scope for this guide.

* Obtain the fingerprint of device certificate.
   * Generate your certificate in PEM format.
   * The fingerprint of the certificate can be either SHA256 or SHA1.
   * You can execute ``` openssl x509 -noout -fingerprint -inform pem -in snxXXXX_device.pem ``` if openssl is installed.
   * The contents of snxXXXX_device.pem can be pasted into an online
  
Fingerprint calculator such as [this one](https://www.samltool.com/fingerprint.php). 
Note that publishing the device certificate or fingerprint online is not technically unsafe 
because the certificate must be paired by the private key derived data during authentication. 
The private key is securely stored on the device and cannot be accessed even programmatically.
The only information that may potentially "leak" by using a public web site in this fashion is the informational 
manufacturer data, including the device serial number. Below is a sample screenshot:

<img src="media/fingerprint.png" width="50%" height="50%" />