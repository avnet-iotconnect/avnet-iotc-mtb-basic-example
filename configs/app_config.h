//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
//

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

// see IotConnectConnectionType: IOTC_CT_AWS or IOTC_CT_AZURE
#define IOTCONNECT_CONNECTION_TYPE IOTC_CT_UNDEFINED
#define IOTCONNECT_CPID "your-cpid"
#define IOTCONNECT_ENV  "your-env"

// prefix for the auto-generated name based on chip HWUID
#define IOTCONNECT_DUID_PREFIX "psoc6-"

// PEM format certificate and private key, if using X509 type auth.
// For example:
#define IOTCONNECT_DEVICE_CERT \
"-----BEGIN CERTIFICATE-----\n" \
"-----END CERTIFICATE-----"


#define IOTCONNECT_DEVICE_KEY \
"-----BEGIN PRIVATE KEY-----\n" \
"-----END PRIVATE KEY-----"

// you can choose to use your own NTP server to obtain network time, or simply time.google.com for better stability
#define IOTCONNECT_SNTP_SERVER "pool.ntp.org"


#endif // APP_CONFIG_H
