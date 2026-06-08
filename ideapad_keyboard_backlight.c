#define WIN32_LEAN_AND_MEAN
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

#define ITS_MODE_NONE               -1
#define ITS_MODE_AUTO               0
#define ITS_MODE_COOL               1
#define ITS_MODE_PERFORMANCE        2
#define ITS_MODE_GEEK               3

#define ITS_MODE_SCMSG_DISABLE      134
#define ITS_MODE_SCMSG_ENABLE       135
#define ITS_MODE_SCMSG_COOL         146
#define ITS_MODE_SCMSG_PERFORMANCE  148
#define ITS_MODE_SCMSG_INTELLIGENT  163
#define ITS_MODE_SCMSG_BSM          164
#define ITS_MODE_SCMSG_EPM          165
#define ITS_MODE_SCMSG_GEEK         172

#define DISPATCHER_VERSION_3        8192

#define SERVICE_NAME_ITS            "LITSSVC"
#define SERVICE_NAME_DISPATCHER     "LenovoProcessManagement"

#define REG_KEY_BIOS                "HARDWARE\\DESCRIPTION\\System\\BIOS"
#define REG_KEY_LITSSVC_IC          "SYSTEM\\CurrentControlSet\\Services\\LITSSVC\\LNBITS\\IC"
#define REG_KEY_LITSSVC_MMC         "SYSTEM\\CurrentControlSet\\Services\\LITSSVC\\LNBITS\\IC\\MMC"
#define REG_KEY_DISPATCHER          "SYSTEM\\CurrentControlSet\\Services\\LenovoProcessManagement\\Performance\\PowerSlider"

#define REG_VAL_SYSFAMILY           "SystemFamily"
#define REG_VAL_VERSION             "Version"
#define REG_VAL_CAPABILITY          "Capability"
#define REG_VAL_AUTO_SETTING        "AutomaticModeSetting"
#define REG_VAL_CURRENT_SETTING     "CurrentSetting"
#define REG_VAL_ITS_FN_CAP          "ITS_FN_Capability"
#define REG_VAL_ITS_CUR_SET         "ITS_CurrentSetting"
#define REG_VAL_ITS_CUR_SET_V       "ITS_CurrentSettingV"


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

int get_dispatcher_version()
{
    HKEY hKey = NULL;
    int rv = -1;
    do {
        LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY_DISPATCHER, 0, KEY_QUERY_VALUE, &hKey);
        if (status != ERROR_SUCCESS) {
            break;
        }
        DWORD dwType = 0;
        DWORD dwValue = 0;
        DWORD dwSize = sizeof(DWORD);
        status = RegQueryValueExA(hKey, REG_VAL_VERSION, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
        if (status != ERROR_SUCCESS || dwType != REG_DWORD) {
            break;
        }
        rv = (int) dwValue;
    } while (FALSE);
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return rv;
}

void get_system_family(char *sys_family, int len, int to_lower) {
    HKEY hKey = NULL;
    do {
        LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY_BIOS, 0, KEY_QUERY_VALUE, &hKey);
        if (status != ERROR_SUCCESS) {
            break;
        }
        DWORD dwType = 0;
        DWORD dwSize = 0;
        status = RegQueryValueExA(hKey, REG_VAL_VERSION, NULL, &dwType, NULL, &dwSize);
        if (status != ERROR_MORE_DATA) {
            break;
        }
        char *szValue = malloc(dwSize);
        memset(szValue, 0, dwSize);
        status = RegQueryValueExA(hKey, REG_VAL_VERSION, NULL, &dwType, (LPBYTE) szValue, &dwSize);
        if (status == ERROR_SUCCESS && dwType == REG_SZ) {
            if (to_lower) {
                CharLowerA(szValue);
            }
            strncpy(sys_family, szValue, len-1);
        }
        free(szValue);
    } while (FALSE);
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
}

int control_service(const char *svc_name, int its_mode_scmsg)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    int rv = -1;
    do {
        hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSCManager == NULL) {
            break;
        }
        hService = OpenServiceA(hSCManager, svc_name, SERVICE_QUERY_STATUS | SERVICE_USER_DEFINED_CONTROL);
        if (hService == NULL) {
            break;
        }
        SERVICE_STATUS serviceStatus;
        if (ControlService(hService, (DWORD) its_mode_scmsg, &serviceStatus) == 0) {
            break;
        }
        rv = 0;
    } while (FALSE);
    if (hService != NULL) {
        CloseServiceHandle(hService);
    }
    if (hSCManager != NULL) {
        CloseServiceHandle(hSCManager);
    }
    return rv;
}

int set_its_mode(int its_mode)
{
    int disp_version = get_dispatcher_version();
    if (disp_version >= DISPATCHER_VERSION_3) {
        int its_mode_scmsg = ITS_MODE_SCMSG_INTELLIGENT;
        switch (its_mode) {
        case ITS_MODE_COOL:
            its_mode_scmsg = ITS_MODE_SCMSG_BSM;
            break;
        case ITS_MODE_PERFORMANCE:
            its_mode_scmsg = ITS_MODE_SCMSG_EPM;
            break;
        case ITS_MODE_GEEK:
            its_mode_scmsg = ITS_MODE_SCMSG_GEEK;
            break;
        }
        return control_service(SERVICE_NAME_DISPATCHER, its_mode_scmsg);
    } else {
        int its_mode_scmsg = ITS_MODE_SCMSG_INTELLIGENT;
        switch (its_mode) {
        case ITS_MODE_COOL:
            its_mode_scmsg = ITS_MODE_SCMSG_COOL;
            break;
        case ITS_MODE_PERFORMANCE:
            its_mode_scmsg = ITS_MODE_SCMSG_PERFORMANCE;
            break;
        case ITS_MODE_GEEK:
            its_mode_scmsg = ITS_MODE_SCMSG_GEEK;
            break;
        }
        return control_service(SERVICE_NAME_DISPATCHER, its_mode_scmsg);
    }
}

int get_its_mode()
{
    int rv = ITS_MODE_NONE;
    char sys_family[33] = { 0 };
    get_system_family(sys_family, 33, 1);
    int is_thinkbook = strstr(sys_family, "thinkbook") ? 1 : 0;
    int disp_version = get_dispatcher_version();
    if (disp_version >= DISPATCHER_VERSION_3) {
        HKEY hKey = NULL;
        do {
            LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY_DISPATCHER, 0, KEY_QUERY_VALUE, &hKey);
            if (status != ERROR_SUCCESS) {
                break;
            }
            DWORD dwType = 0;
            DWORD dwValue = 0;
            DWORD dwSize = sizeof(DWORD);
            status = RegQueryValueExA(hKey, REG_VAL_ITS_FN_CAP, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
            if (status != ERROR_SUCCESS || dwType != REG_DWORD) {
                break;
            }
            int capability = (int) dwValue;
            if (!is_thinkbook) {
                capability &= ~0x10;
            }
            int use_versioned = (capability & 0x10) != 0;
            char *setting_key = use_versioned ? REG_VAL_ITS_CUR_SET_V : REG_VAL_ITS_CUR_SET;
            status = RegQueryValueExA(hKey, setting_key, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
            if (status != ERROR_SUCCESS || dwType != REG_DWORD) {
                break;
            }
            int curr_setting = (int) dwValue;
            switch (curr_setting) {
            case 0:
                rv = ITS_MODE_AUTO;
                break;
            case 1:
                rv = ITS_MODE_COOL;
                break;
            case 3:
                rv = ITS_MODE_PERFORMANCE;
                break;
            case 4:
                rv = ITS_MODE_GEEK;
                break;
            }
        } while (FALSE);
        if (hKey != NULL) {
            RegCloseKey(hKey);
        }
    } else {
        HKEY hKey = NULL;
        do {
            LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY_LITSSVC_MMC, 0, KEY_QUERY_VALUE, &hKey);
            if (status != ERROR_SUCCESS) {
                break;
            }
            DWORD dwType = 0;
            DWORD dwValue = 0;
            DWORD dwSize = sizeof(DWORD);
            status = RegQueryValueExA(hKey, REG_VAL_AUTO_SETTING, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
            if (status != ERROR_SUCCESS && dwType != REG_DWORD) {
                break;
            }
            int auto_setting = (int) dwValue;
            status = RegQueryValueExA(hKey, REG_VAL_CURRENT_SETTING, NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
            if (status != ERROR_SUCCESS || dwType != REG_DWORD) {
                break;
            }
            int curr_setting = (int) dwValue;
            if (auto_setting == 2 && curr_setting == 0) {
                rv = ITS_MODE_AUTO;
            }
            if (auto_setting == 1) {
                if (curr_setting == 1) {
                    rv = ITS_MODE_COOL;
                }
                if (curr_setting == 3) {
                    rv = ITS_MODE_PERFORMANCE;
                }
            }
        } while (FALSE);
        if (hKey != NULL) {
            RegCloseKey(hKey);
        }
    }
    return rv;
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
