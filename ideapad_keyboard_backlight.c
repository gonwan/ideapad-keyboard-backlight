#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define IOCTL_BACKLIGHT         0x83102144

#define BACKLIGHT_CAPABILITY    0x01
#define BACKLIGHT_STATUS        0x32
#define BACKLIGHT_OFF           0x00033
#define BACKLIGHT_LEVEL1        0x10033
#define BACKLIGHT_LEVEL2        0x20033
#define BACKLIGHT_AUTO          0x30033


char *get_base_name(char *path)
{
    if (!path) {
        return NULL;
    }
    char *an = NULL;
    char *p = path;
    while (*p++) {
        if (*p == '\\' || *p == '/') {
            an = p;
        }
    }
    return (an == NULL) ? path : (an + 1);
}

uint32_t device_io_control(HANDLE drv_handle, uint32_t func)
{
    char outbuff[4] = { 0 };
    DWORD ret_len = 0;
    if (DeviceIoControl(drv_handle, IOCTL_BACKLIGHT, &func, sizeof(func), outbuff, sizeof(outbuff), &ret_len, NULL)) {
        outbuff[ret_len] = '\0';
        return (outbuff[3] << 24) | (outbuff[2] << 16) | (outbuff[1] << 8) | outbuff[0];
    } else {
        fprintf(stderr, "Error: DeviceIoControl failed, func=%d\n", func);
        return (uint32_t) -1;
    }
}

uint32_t get_backlight_capability(HANDLE drv_handle)
{
    return device_io_control(drv_handle, BACKLIGHT_CAPABILITY);
}

uint32_t get_backlight_status(HANDLE drv_handle)
{
    return device_io_control(drv_handle, BACKLIGHT_STATUS);
}

uint32_t set_backlight_level(HANDLE drv_handle, int level)
{
    uint32_t func = BACKLIGHT_AUTO;
    switch (level) {
        case 0:
            func = BACKLIGHT_OFF;
            break;
        case 1:
            func = BACKLIGHT_LEVEL1;
            break;
        case 2:
            func = BACKLIGHT_LEVEL2;
            break;
    }
    return device_io_control(drv_handle, func);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [0|1|2|3]\n", get_base_name(argv[0]));
        return -1;
    }
    int level = (int) strtol(argv[1], NULL, 0);
    if (level < 0 || level > 3) {
        fprintf(stderr, "Usage: %s [0|1|2|3]\n", get_base_name(argv[0]));
        return -1;
    }
    /* open */
    HANDLE drv_handle = CreateFileA("\\\\.\\EnergyDrv", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (drv_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Failed to find device, make sure Lenovo energy management driver is installed!\n");
        return -1;
    }
    /* run */
    do {
        /* cap */
        uint32_t cap = get_backlight_capability(drv_handle);
        if (cap == (uint32_t) -1) {
            break;
        }
        if ((cap & 1) != 1) {
            fprintf(stderr, "Error: Failed to get capability!\n");
            break;
        }
        if ((cap >> 1) != 3) {
            fprintf(stderr, "Error: Auto-level not supported!\n");
            break;
        }
        /* status */
        uint32_t status = get_backlight_status(drv_handle);
        if (status == (uint32_t) -1) {
            break;
        }
        if ((status & 1) != 1) {
            fprintf(stderr, "Error: Failed to get current status!\n");
            break;
        }
        status = status >> 1;
        if ((status & 0x8000) != 0x8000) {
            fprintf(stderr, "Error: Cannot set backlight when keyboard is disabled!\n");
            break;
        }
        int curr_level = -1;
        switch (status & 0x7fff) {
            case 0:
                curr_level = 0;
                break;
            case 1:
                curr_level = 1;
                break;
            case 2:
                curr_level = 2;
                break;
            case 3:
                curr_level = 3;
                break;
        }
        printf("Current level: %d\n", curr_level);
        set_backlight_level(drv_handle, level);
        printf("Finished setting level: %d\n", level);
    } while (FALSE);
    /* close */
    CloseHandle(drv_handle);
    return 0;
}
