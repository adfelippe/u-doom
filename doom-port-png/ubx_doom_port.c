#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "doomkeys.h"
#include "doomgeneric.h"
#include "lodepng.h"

#define FRAME_SIZE      (DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(uint32_t))

static int count = 1;

void DG_Init()
{
}

void DG_DrawFrame(void)
{
    char filename[15] = {0};
    uint8_t *pScreenBuffer = (uint8_t *)DG_ScreenBuffer;
    uint8_t pImageBuffer[FRAME_SIZE];

    for (uint32_t i = 0; i < FRAME_SIZE; i += 4) {
        pImageBuffer[i] = pScreenBuffer[i + 2];
        pImageBuffer[i + 1] = pScreenBuffer[i + 1];
        pImageBuffer[i + 2] = pScreenBuffer[i];
        pImageBuffer[i + 3] = 0xFF;
    }

    if (count < 4) {
        sprintf(filename, "doom%d.png", count);
        printf("Creating %s\n", filename);
        unsigned int error = lodepng_encode32_file(filename, pImageBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
        /*if there's an error, display it*/
        if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
        ++count;
    } else {
        uint8_t *png;
        size_t pngSize;
        lodepng_encode32(&png, &pngSize, pImageBuffer, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
        printf("pngSize = %lu\n", pngSize);
        free(png);
    }
    DG_SleepMs(250);
}

void DG_SleepMs(uint32_t ms)
{
    usleep(ms * 1000);
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
        doomgeneric_Tick();
    }

    return 0;
}
