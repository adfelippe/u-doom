#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "ubxlib.h"
#include "doomkeys.h"
#include "doomgeneric.h"

#define DOOM_FRAME_SIZE     (DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4)
#define SINGLE_PACKET_SIZE  200
#define PACKETS_TO_SEND     (DOOM_FRAME_SIZE / SINGLE_PACKET_SIZE)

const char gStartOfFrame[] = {0xCA, 0xFE, 0xBA, 0xBE};
const char gEndOfFrame[] = {0xDE, 0xAD, 0xBE, 0xEF};

static uDeviceType_t gDeviceType = U_DEVICE_TYPE_SHORT_RANGE;
static const uNetworkCfgBle_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_BLE,
    .role = U_BLE_CFG_ROLE_PERIPHERAL,
    .spsServer = true
};
static uDeviceCfg_t gDeviceCfg;
static volatile bool gIsConnected = false;
static uint16_t gCharHandle = -1;
static int32_t gSpsChannel = -1;
static int32_t gMtuSize = 0;
uDeviceHandle_t gDeviceHandle;


static void connectionCallback(int32_t connHandle, char *address, int32_t status,
                               int32_t channel, int32_t mtu, void *pParameters)
{
    printf("connectionCallback\n");
    if (status == (int32_t)U_BLE_SPS_CONNECTED) {
        printf("Connected to: %s, channel: %d, mtu: %d\n", address, channel, mtu);
        gIsConnected = true;
        gSpsChannel = channel;
        gMtuSize = mtu;
    } else if (status == (int32_t)U_BLE_SPS_DISCONNECTED) {
        if (connHandle != U_BLE_SPS_INVALID_HANDLE) {
            printf("Disconnected\n");
            gIsConnected = false;
        } else {
            printf("* Connection attempt failed\n");
        }
    }
}

void DG_Init()
{
    // Remove the line below if you want the log printouts from ubxlib
    //uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox module
    int32_t errorCode;

    uDeviceGetDefaults(gDeviceType, &gDeviceCfg);
    gDeviceCfg.deviceCfg.cfgSho.moduleType = U_SHORT_RANGE_MODULE_TYPE_NINA_W15;
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &gDeviceHandle);

    if (errorCode == 0) {
        printf("Bringing up the BLE network...\n");
        errorCode = uNetworkInterfaceUp(gDeviceHandle, gNetworkCfg.type, &gNetworkCfg);

        if (errorCode == 0) {
            uBleSpsSetCallbackConnectionStatus(gDeviceHandle, connectionCallback, &gDeviceHandle);
            printf("Waiting for connections...\n");
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }
}

void DG_DrawFrame()
{
    if (gIsConnected) {
        uBleSpsSend(gDeviceHandle, gSpsChannel, gStartOfFrame, sizeof(gStartOfFrame));
        for (uint32_t i = 0; i < PACKETS_TO_SEND; i += SINGLE_PACKET_SIZE) {
            uBleSpsSend(gDeviceHandle, gSpsChannel, (const char*)&DG_ScreenBuffer[i], SINGLE_PACKET_SIZE);
        }
        uBleSpsSend(gDeviceHandle, gSpsChannel, gEndOfFrame, sizeof(gEndOfFrame));
    }
}

void DG_SleepMs(uint32_t ms)
{
    usleep (ms * 1000);
}

uint32_t DG_GetTicksMs()
{
    struct timeval  tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);
    /* return milliseconds */
    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000); 
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    // TODO
    return 0;
}

void DG_SetWindowTitle(const char * title)
{
    // TODO
}

int main(int argc, char **argv)
{
    printf("Starting u-doom, a u-blox doom port via BLE streaming\n");
    doomgeneric_Create(argc, argv);

    for (;;) {
        if (gIsConnected) {
            doomgeneric_Tick();
        }
    }

    return 0;
}
