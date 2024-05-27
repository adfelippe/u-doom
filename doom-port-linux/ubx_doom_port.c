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
#define DOOM_FRAME_SIZE         (DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4)
#define SINGLE_PACKET_SIZE      244
#define TX_SLEEP_MS             1
#define TX_SLEEP_US             3000
#define KEY_QUEUE_SIZE          100

enum { 
    UP_KEY = 38,
    DOWN_KEY = 40,
    LEFT_KEY = 37,
    RIGHT_KEY = 39,
    CTRL_KEY = 17,
    ENTER_KEY = 13,
    SPACE_KEY = 32,
    ESCAPE_KEY = 27
};

char gStartOfFrame[] = {0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0, 0};
const char gEndOfFrame[] = {0xDE, 0xAD, 0xBE, 0xEF};
// Button frame format:
// [HEADER][PRESSED][KEY]
const uint8_t gButtonFrameHeader[] = {0xAB, 0xCD};
const uint8_t gAckFrame[] = {0xFE, 0xED};

typedef struct uKeyData {
    bool isPressed;
    uint8_t key;
} uKeyData_t;

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
static uPortQueueHandle_t gKeyQueueHandle;
static uint32_t gFrameCount = 0;
static uint32_t gStartTimeMs = 0;
static float gElapsedTimeSec = 0.0F;

static uint8_t convertToDoomKey(uint8_t receivedKey);

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
    uint8_t buffer[SINGLE_PACKET_SIZE + 1] = {0};
    uDeviceHandle_t *pDeviceHandle = (uDeviceHandle_t *)pParameters;
    int32_t length = uBleSpsReceive(*pDeviceHandle, channel, (char *)buffer, sizeof(buffer) - 1);

    if (length >= 4 && buffer[0] == gButtonFrameHeader[0] && buffer[1] == gButtonFrameHeader[1]) {
        uKeyData_t keyData = {.isPressed = buffer[2], .key = convertToDoomKey(buffer[3])};
        uPortQueueSend(gKeyQueueHandle, &keyData);
        //printf("Key pressed: %u, value: %u\n", buffer[2], buffer[3]);
    } else {
        printf("Woops... length: %d, buffer[0] = %02X, buffer[1] = %02X\n", length, buffer[0], buffer[1]);
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

static uint8_t convertToDoomKey(uint8_t receivedKey)
{
    uint8_t key;

    switch (receivedKey) {
    case ENTER_KEY:
        key = KEY_ENTER;
        break;
    case LEFT_KEY:
        key = KEY_LEFTARROW;
        break;
    case RIGHT_KEY:
        key = KEY_RIGHTARROW;
        break;
    case UP_KEY:
        key = KEY_UPARROW;
        break;
    case DOWN_KEY:
        key = KEY_DOWNARROW;
        break;
    case CTRL_KEY:
        key = KEY_FIRE;
        break;
    case SPACE_KEY:
        key = KEY_USE;
        break;
    case ESCAPE_KEY:
        key = KEY_ESCAPE;
        break;
    default:
        key = 0xFF;
        break;
    }
    
    return key;
}

void DG_Init()
{
    int32_t errorCode;

    // Initiate ubxlib
    uPortInit();
    uDeviceInit();

    errorCode = uPortQueueCreate(KEY_QUEUE_SIZE, sizeof(uKeyData_t), &gKeyQueueHandle);
    if (errorCode != 0) { 
        printf("Failed to create queue: %d\n", errorCode);
    } else {
        printf("Key queue created successfully!\n");
    }

    if (errorCode == 0) {
        uDeviceGetDefaults(gDeviceType, &gDeviceCfg);
        gDeviceCfg.deviceCfg.cfgSho.moduleType = U_SHORT_RANGE_MODULE_TYPE_NINA_W15;
        printf("\nInitiating the module...\n");
        errorCode = uDeviceOpen(&gDeviceCfg, &gDeviceHandle);
    }

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
        float fps;
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
        //printf("pngSize = %zu\n", pngSize);

        if (pngSize) {
            uint32_t packetsToSend = pngSize / gMtuSize;
            uint32_t remainder = pngSize % gMtuSize;
            uint32_t offset = 0;
            uint8_t *pPngSizeArray = (uint8_t *)&pngSize;

            // Copy PNG size in bytes to start of frame - remote will expect that number of bytes
            gStartOfFrame[4] = pPngSizeArray[3];
            gStartOfFrame[5] = pPngSizeArray[2];
            gStartOfFrame[6] = pPngSizeArray[1];
            gStartOfFrame[7] = pPngSizeArray[0];

            if (gIsFirstPacket) {
                printf("Waiting a few seconds before sending the first package...\n");
                DG_SleepMs(5000);
                gIsFirstPacket = false;
                gStartTimeMs = DG_GetTicksMs();
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

            ++gFrameCount;
            fps = (float)gFrameCount / ((float)(DG_GetTicksMs() - gStartTimeMs) / 1000.0F);
            printf("FPS: %.2f\n", fps);
            free(pPngArray);
        }
    } else {
        // Roughly 35 FPS
        DG_SleepMs(29);
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
    int hasKey = 0;

    if (uPortQueueGetFree(gKeyQueueHandle) < KEY_QUEUE_SIZE) {
        uKeyData_t keyData;
        uPortQueueReceive(gKeyQueueHandle, &keyData);
        *pressed = (int)keyData.isPressed;
        *doomKey = (unsigned char)keyData.key;
        hasKey = 1;
    }

    return hasKey;
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
