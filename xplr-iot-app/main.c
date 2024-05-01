
/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Simple BLE SPS example using ubxlib
 *
 * The only ubxlib BLE functionality available for now
 * is the UBlox SPS service. This example implements a
 * SPS echo server to which a client can connect and
 * send data and then get that data echoed back.
 * A typical client can be the "U-blox Bluetooth Low Energy"
 * application available for Android and IOS.
 *
 */

#include <stdio.h>
#include <string.h>
#include "ubxlib.h"

static uDeviceType_t gDeviceType = U_DEVICE_TYPE_SHORT_RANGE;
static const uNetworkCfgBle_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_BLE,
    .role = U_BLE_CFG_ROLE_PERIPHERAL,
    .spsServer = false
};
static uDeviceCfg_t gDeviceCfg;
static volatile bool gIsConnected = false;
static uint16_t gCharHandle = -1;
static int32_t gConnHandle = -1;

static void gapConnectionCallback(int32_t connHandle, char *pAddress, bool connected)
{
    gIsConnected = connected;
    gConnHandle = connHandle;

    if (connected) {
        printf("GAP connected to: %s\n", pAddress);
    } else {
        printf("GAP disconnected\n");
    }
}

void main()
{
    // Remove the line below if you want the log printouts from ubxlib
    //uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox module
    int32_t errorCode;
    uDeviceHandle_t deviceHandle;

    uDeviceGetDefaults(gDeviceType, &gDeviceCfg);
    gDeviceCfg.deviceCfg.cfgSho.moduleType = U_SHORT_RANGE_MODULE_TYPE_NINA_W15;
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &deviceHandle);

    if (errorCode == 0) {
        printf("Bringing up the ble network...\n");
        errorCode = uNetworkInterfaceUp(deviceHandle, gNetworkCfg.type, &gNetworkCfg);

        if (errorCode == 0) {
            uBleGapSetConnectCallback(deviceHandle, gapConnectionCallback);
            uBleGattBeginAddService(deviceHandle, "2456e1b926e28f83e744f34f01e9d701");
            uBleGattAddCharacteristic(deviceHandle, "2456e1b926e28f83e744f34f01e9d703", 0x10, &gCharHandle);
            uBleGattEndAddService(deviceHandle);

            printf("\n== Start a SPS client e.g. in a phone ==\n\n");
            printf("Waiting for connections...\n");
            while (1) {
                uPortTaskBlock(2000);
                if (gIsConnected) {
                    uBleGattWriteNotifyValue(deviceHandle, gConnHandle, gCharHandle, "HEY!", 4);
                }
            }
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }

        uDeviceClose(deviceHandle, true);
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }
}
