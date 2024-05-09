#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "ubxlib.h"
#include "doomkeys.h"
#include "doomgeneric.h"
#include "lodepng.h"

// X * Y * 4 (RGBA size)
#define DOOM_FRAME_SIZE     (DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4)
#define SINGLE_PACKET_SIZE  242

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

static size_t covertFrameToPng(uint8_t *pPngArray) {
    size_t pngSize;
    uint8_t *pScreenBuffer = (uint8_t *)DG_ScreenBuffer;
    uint8_t pImageBuffer[DOOM_FRAME_SIZE];

    for (uint32_t i = 0; i < DOOM_FRAME_SIZE; i += 4) {
        pImageBuffer[i] = pScreenBuffer[i + 2];
        pImageBuffer[i + 1] = pScreenBuffer[i + 1];
        pImageBuffer[i + 2] = pScreenBuffer[i];
        pImageBuffer[i + 3] = 0xFF;
    }

    uint32_t error = lodepng_encode32(&pPngArray, &pngSize, pImageBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    if (error) {
        printf("lodepng error %u: %s\n", error, lodepng_error_text(error));
        pngSize = 0;
    }

    return pngSize;
}

static void sendBle(const uint8_t *data, uint32_t size)
{
    if (gIsConnected) {
        uBleSpsSend(gDeviceHandle, gSpsChannel, data, size);
    }
}

static void prepareImageBuffer(uint8_t *pImageBuffer, uint32_t bufferSize) {
    uint8_t *pScreenBuffer = (uint8_t *)DG_ScreenBuffer;
    
    for (uint32_t i = 0; i < bufferSize; i += 4) {
        pImageBuffer[i] = pScreenBuffer[i + 2];
        pImageBuffer[i + 1] = pScreenBuffer[i + 1];
        pImageBuffer[i + 2] = pScreenBuffer[i];
        pImageBuffer[i + 3] = 0xFF;
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

void DG_DrawFrame(void)
{
    if (gIsConnected) {
        uint8_t pImageBuffer[DOOM_FRAME_SIZE];
        uint8_t *pPngArray;
        size_t pngSize;
        uint32_t error;

        prepareImageBuffer(pImageBuffer, DOOM_FRAME_SIZE);
        error = lodepng_encode32(&pPngArray, &pngSize, pImageBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
        if (error) {
            printf("lodepng error %u: %s\n", error, lodepng_error_text(error));
            pngSize = 0;
        }
        printf("pngSize = %lu\n", pngSize);

        if (pngSize) {
            uint32_t packetsToSend = pngSize / SINGLE_PACKET_SIZE;
            uint32_t remainder = pngSize % SINGLE_PACKET_SIZE;
            uint32_t offset = 0;

            // printf("packetsToSend = %u\n", packetsToSend);
            // printf("remainder = %u\n", remainder);

            sendBle(gStartOfFrame, sizeof(gStartOfFrame));

            for (uint32_t i = 0; i < packetsToSend; ++i) {
                sendBle(&pPngArray[offset], SINGLE_PACKET_SIZE);
                offset += SINGLE_PACKET_SIZE;
                DG_SleepMs(10);
            }

            if (remainder) {
                sendBle(&pPngArray[offset], remainder);
            }

            sendBle(gEndOfFrame, sizeof(gEndOfFrame));
            free(pPngArray);
        }
    } else {
        DG_SleepMs(500);
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
    doomgeneric_Create(argc, argv);

    for (;;) {
        if (gIsConnected) {
            doomgeneric_Tick();
        }
    }

    return 0;
}
