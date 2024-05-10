#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sysinfoapi.h>
#include "ubxlib.h"
#include "doomkeys.h"
#include "doomgeneric.h"
#include "lodepng.h"
#include "usleep.h"

// X * Y * 4 (RGBA size)
#define DOOM_FRAME_SIZE         (DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4)
#define SINGLE_PACKET_SIZE      244
#define SEMAPHORE_TIMEOUT_MS    1000
#define TX_SLEEP_MS             1
#define TX_SLEEP_US             10000

char gStartOfFrame[] = {0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0, 0};
const char gEndOfFrame[] = {0xDE, 0xAD, 0xBE, 0xEF};

static uDeviceType_t gDeviceType = U_DEVICE_TYPE_SHORT_RANGE;
static const uNetworkCfgBle_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_BLE,
    .role = U_BLE_CFG_ROLE_PERIPHERAL,
    .spsServer = true
};
static uDeviceCfg_t gDeviceCfg;
static volatile bool gIsConnected = false;
static volatile bool gIsFirstPacket = true;
static uint16_t gCharHandle = -1;
static int32_t gSpsChannel = -1;
static int32_t gMtuSize = 0;
static uDeviceHandle_t gDeviceHandle;
static uPortSemaphoreHandle_t gTxSem;


static void connectionCallback(int32_t connHandle, char *address, int32_t status,
                               int32_t channel, int32_t mtu, void *pParameters)
{
    if (status == (int32_t)U_BLE_SPS_CONNECTED) {
        uBleSpsSetSendTimeout(gDeviceHandle, gSpsChannel, 500);
        gSpsChannel = channel;
        gMtuSize = mtu;
        gIsConnected = true;
        printf("Connected to: %s, channel: %d, mtu: %d\n", address, channel, mtu);
    } else if (status == (int32_t)U_BLE_SPS_DISCONNECTED) {
        if (connHandle != U_BLE_SPS_INVALID_HANDLE) {
            gIsConnected = false;
            gIsFirstPacket = true;
            printf("Disconnected\n");
        } else {
            printf("Connection attempt failed\n");
        }
    }
}

static void dataAvailableCallback(int32_t channel, void *pParameters)
{
    char buffer[SINGLE_PACKET_SIZE] = {0};
    uDeviceHandle_t *pDeviceHandle = (uDeviceHandle_t *)pParameters;
    int32_t length = uBleSpsReceive(*pDeviceHandle, channel, buffer, sizeof(buffer) - 1);

    printf("dataAvailableCallback\n");
    if (length >= 2) {
        printf("Received data: %.2X %.2X\n", buffer[0], buffer[1]);
        uPortSemaphoreGive(gTxSem);
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
        int32_t bytesSent = 0;
        while (bytesSent < size) {
            bytesSent = uBleSpsSend(gDeviceHandle, gSpsChannel, &data[bytesSent], size - bytesSent);
        }
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
    int32_t errorCode = uPortSemaphoreCreate(&gTxSem, 0, 1);

    if (errorCode != 0) { 
        printf("Failed to create semaphore: %d\n", errorCode);
    }

    uDeviceGetDefaults(gDeviceType, &gDeviceCfg);
    gDeviceCfg.deviceCfg.cfgSho.moduleType = U_SHORT_RANGE_MODULE_TYPE_NINA_W15;
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &gDeviceHandle);

    if (errorCode == 0) {
        printf("Bringing up the BLE network...\n");
        errorCode = uNetworkInterfaceUp(gDeviceHandle, gNetworkCfg.type, &gNetworkCfg);

        if (errorCode == 0) {
            uBleSpsSetCallbackConnectionStatus(gDeviceHandle, connectionCallback, &gDeviceHandle);
            uBleSpsSetDataAvailableCallback(gDeviceHandle, dataAvailableCallback, &gDeviceHandle);
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
        printf("pngSize = %zu\n", pngSize);

        if (pngSize) {
            uint32_t packetsToSend = pngSize / gMtuSize;
            uint32_t remainder = pngSize % gMtuSize;
            uint32_t offset = 0;
            uint8_t *pPngSizeArray = (uint8_t *)&pngSize;

            //printf("packetsToSend = %u\n", packetsToSend);
            //printf("remainder = %u\n", remainder);

            // Copy PNG size in bytes to start of frame - remote will expect that number of bytes
            gStartOfFrame[4] = pPngSizeArray[3];
            gStartOfFrame[5] = pPngSizeArray[2];
            gStartOfFrame[6] = pPngSizeArray[1];
            gStartOfFrame[7] = pPngSizeArray[0];

            if (gIsFirstPacket) {
                printf("Waiting a few seconds before sending the first package...\n");
                DG_SleepMs(5000);
                gIsFirstPacket = false;
            }

            sendBle(gStartOfFrame, sizeof(gStartOfFrame));
            usleep(TX_SLEEP_US);

            for (uint32_t i = 0; i < packetsToSend; ++i) {
                sendBle(&pPngArray[offset], gMtuSize);
                offset += gMtuSize;
                usleep(TX_SLEEP_US);
            }

            if (remainder) {
                sendBle(&pPngArray[offset], remainder);
                usleep(TX_SLEEP_US);
            }

            free(pPngArray);
        }
    } else {
        // Roughly 35 FPS
        DG_SleepMs(29);
    }
}

void DG_SleepMs(uint32_t ms)
{
    Sleep(ms);
}

uint32_t DG_GetTicksMs()
{
    return GetTickCount();
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
