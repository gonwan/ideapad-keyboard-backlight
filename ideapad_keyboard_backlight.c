#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define IOCTL_BACKLIGHT             0x83102144
#define IOCTL_BATTERY_CHARGE        0x831020f8
#define IOCTL_BATTERY_CHARGE_NIGHT  0x83102150

#define BACKLIGHT_CAPABILITY        0x01
#define BACKLIGHT_STATUS            0x32
#define BACKLIGHT_OFF               0x00033
#define BACKLIGHT_LEVEL1            0x10033
#define BACKLIGHT_LEVEL2            0x20033
#define BACKLIGHT_AUTO              0x30033

#define CHARGE_STATUS               0xffffffff  /* 0xff */
#define CHARGE_QUICK_ON             0x07
#define CHARGE_QUICK_OFF            0x08
#define CHARGE_STORAGE_ON           0x03
#define CHARGE_STORAGE_OFF          0x05
#define CHARGE_NIGHT_STATUS         0x11
#define CHARGE_NIGHT_ON             0x80000012
#define CHARGE_NIGHT_OFF            0x12

#define ITS_SCMSG_DISABLE           0x86
#define ITS_SCMSG_ENABLE            0x87
#define ITS_SCMSG_COOL              0x92
#define ITS_SCMSG_PERFORMANCE       0x94
#define ITS_SCMSG_INTELLIGENT       0xa3
#define ITS_SCMSG_BSM               0xa4
#define ITS_SCMSG_EPM               0xa5
#define ITS_SCMSG_GEEK              0xac

#define DISPATCHER_VERSION_3        8192

#define SERVICE_NAME_ITS            "LITSSVC"
#define SERVICE_NAME_DISPATCHER     "LenovoProcessManagement"

#define REG_KEY_BIOS                "HARDWARE\\DESCRIPTION\\System\\BIOS"
#define REG_KEY_CHARGE_LEGACY       "Software\\Lenovo\\iMController\\Contracts\\SystemManagement.BatteryMgmt"   /* HKCU */ 
#define REG_KEY_CHARGE_VANTAGE      "Software\\Lenovo\\VantageService\\AddinData\\IdeaNotebookAddin"            /* HKCU */
#define REG_KEY_LITSSVC_IC          "SYSTEM\\CurrentControlSet\\Services\\LITSSVC\\LNBITS\\IC"
#define REG_KEY_LITSSVC_MMC         "SYSTEM\\CurrentControlSet\\Services\\LITSSVC\\LNBITS\\IC\\MMC"
#define REG_KEY_DISPATCHER          "SYSTEM\\CurrentControlSet\\Services\\LenovoProcessManagement\\Performance\\PowerSlider"

#define REG_VAL_SYSFAMILY           "SystemFamily"
#define REG_VAL_CHARGE_MODE         "BatteryChargeMode"
#define REG_VAL_VERSION             "Version"
#define REG_VAL_CAPABILITY          "Capability"
#define REG_VAL_AUTO_SETTING        "AutomaticModeSetting"
#define REG_VAL_CURRENT_SETTING     "CurrentSetting"
#define REG_VAL_ITS_FN_CAP          "ITS_FN_Capability"
#define REG_VAL_ITS_CUR_SET         "ITS_CurrentSetting"
#define REG_VAL_ITS_CUR_SET_V       "ITS_CurrentSettingV"


typedef enum _backlight_level_e {
    BACKLIGHT_LEVEL_NONE = -1,
    BACKLIGHT_LEVEL_OFF,
    BACKLIGHT_LEVEL_1,
    BACKLIGHT_LEVEL_2,
    BACKLIGHT_LEVEL_AUTO
} backlight_level_e;

typedef enum _charge_mode_e {
    CHARGE_MODE_NONE = -1,
    CHARGE_MODE_NORMAL,
    CHARGE_MODE_QUICK,
    CHARGE_MODE_STORAGE,
    CHARGE_MODE_NIGHT
} charge_mode_e;

typedef enum _its_mode_e {
    ITS_MODE_NONE = -1,
    ITS_MODE_AUTO,
    ITS_MODE_COOL,
    ITS_MODE_PERFORMANCE,
    ITS_MODE_GEEK  /* not working on my XiaoXinPro 14 model */
} its_mode_e;


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

uint32_t device_io_control(HANDLE drv_handle, uint32_t ctl, uint32_t func)
{
    uint32_t rv = 0;
    DWORD len = 0;
    if (DeviceIoControl(drv_handle, ctl, &func, sizeof(func), &rv, sizeof(rv), &len, NULL)) {
        return rv;
    } else {
        fprintf(stderr, "Error: DeviceIoControl failed, func=%d\n", func);
        return (uint32_t) -1;
    }
}

uint32_t get_backlight_capability(HANDLE drv_handle)
{
    return device_io_control(drv_handle, IOCTL_BACKLIGHT, BACKLIGHT_CAPABILITY);
}

uint32_t get_backlight_status(HANDLE drv_handle)
{
    return device_io_control(drv_handle, IOCTL_BACKLIGHT, BACKLIGHT_STATUS);
}

uint32_t set_backlight_level(HANDLE drv_handle, backlight_level_e level)
{
    uint32_t func = BACKLIGHT_AUTO;
    switch (level) {
        case BACKLIGHT_LEVEL_OFF:
            func = BACKLIGHT_OFF;
            break;
        case BACKLIGHT_LEVEL_1:
            func = BACKLIGHT_LEVEL1;
            break;
        case BACKLIGHT_LEVEL_2:
            func = BACKLIGHT_LEVEL2;
            break;
    }
    return device_io_control(drv_handle, IOCTL_BACKLIGHT, func);
}

uint32_t get_charge_mode(HANDLE drv_handle, int is_night)
{
    if (is_night) {
        return device_io_control(drv_handle, IOCTL_BATTERY_CHARGE_NIGHT, CHARGE_NIGHT_STATUS);
    } else {
        return device_io_control(drv_handle, IOCTL_BATTERY_CHARGE, CHARGE_STATUS);
    }
}

uint32_t set_charge_mode(HANDLE drv_handle, charge_mode_e curr_mode, charge_mode_e mode)
{
    if (curr_mode == mode) {
        return 0;
    }
    switch (curr_mode) {
    case CHARGE_MODE_QUICK:
        device_io_control(drv_handle, IOCTL_BATTERY_CHARGE, CHARGE_QUICK_OFF);
        break;
    case CHARGE_MODE_STORAGE:
        device_io_control(drv_handle, IOCTL_BATTERY_CHARGE, CHARGE_STORAGE_OFF);
        break;
    case CHARGE_MODE_NIGHT:
        device_io_control(drv_handle, IOCTL_BATTERY_CHARGE_NIGHT, CHARGE_NIGHT_OFF);
        break;
    }
    uint32_t rv = 0;
    switch (mode) {
    case CHARGE_MODE_QUICK:
        rv = device_io_control(drv_handle, IOCTL_BATTERY_CHARGE, CHARGE_QUICK_ON);
        break;
    case CHARGE_MODE_STORAGE:
        rv = device_io_control(drv_handle, IOCTL_BATTERY_CHARGE, CHARGE_STORAGE_ON);
        break;
    case CHARGE_MODE_NIGHT:
        rv = device_io_control(drv_handle, IOCTL_BATTERY_CHARGE_NIGHT, CHARGE_NIGHT_ON);
        break;
    }
    return rv;
}

int set_charge_mode_registry(charge_mode_e mode)
{
    HKEY hKey = NULL;
    int rv = -1;
    do {
        int is_legacy = 1;
        LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY_CHARGE_LEGACY, 0, KEY_SET_VALUE, &hKey);
        if (status != ERROR_SUCCESS) {
            is_legacy = 0;
            status = RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY_CHARGE_VANTAGE, 0, KEY_SET_VALUE, &hKey);
            if (status != ERROR_SUCCESS) {
                break;
            }
        }
        const char *szMode = "Normal";
        switch (mode) {
        case CHARGE_MODE_QUICK:
            szMode = "Quick";
            break;
        case CHARGE_MODE_STORAGE:
            szMode = "Storage";
            break;
        case CHARGE_MODE_NIGHT:
            if (is_legacy) {
                szMode = "Night";
            }
            break;
        }
        status = RegSetValueExA(hKey, REG_VAL_CHARGE_MODE, 0, REG_EXPAND_SZ, szMode, strlen(szMode));
        if (status != ERROR_SUCCESS) {
            break;
        }
        rv = 0;
    } while (0);
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return rv;
}

void get_system_family(char *sys_family, int len) {
    HKEY hKey = NULL;
    do {
        LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY_BIOS, 0, KEY_QUERY_VALUE, &hKey);
        if (status != ERROR_SUCCESS) {
            break;
        }
        DWORD dwType = 0;
        DWORD dwSize = 0;
        status = RegQueryValueExA(hKey, REG_VAL_SYSFAMILY, NULL, &dwType, NULL, &dwSize);
        if (status != ERROR_SUCCESS) {
            break;
        }
        char *szValue = malloc(dwSize);
        memset(szValue, 0, dwSize);
        status = RegQueryValueExA(hKey, REG_VAL_SYSFAMILY, NULL, &dwType, (LPBYTE) szValue, &dwSize);
        if (status == ERROR_SUCCESS && dwType == REG_SZ) {
            strncpy(sys_family, szValue, len-1);
        }
        free(szValue);
    } while (0);
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
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
    } while (0);
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return rv;
}

int control_service(const char *svc_name, int its_scmsg)
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
            fprintf(stderr, "Error: Failed to find service: %s\n", svc_name);
            break;
        }
        SERVICE_STATUS serviceStatus;
        if (ControlService(hService, (DWORD) its_scmsg, &serviceStatus) == 0) {
            /* ERROR_SERVICE_NOT_ACTIVE or ERROR_SERVICE_CANNOT_ACCEPT_CTRL possible when run from task scheduler */
            fprintf(stderr, "Error: Failed to control service: %s(%d)\n", svc_name, GetLastError());
            break;
        }
        rv = 0;
    } while (0);
    if (hService != NULL) {
        CloseServiceHandle(hService);
    }
    if (hSCManager != NULL) {
        CloseServiceHandle(hSCManager);
    }
    return rv;
}

its_mode_e get_its_mode(const char *model)
{
    its_mode_e rv = ITS_MODE_NONE;
    char *model_lower = strdup(model);
    CharLowerA(model_lower);
    int is_thinkbook = strstr(model_lower, "thinkbook") ? 1 : 0;
    free(model_lower);
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
        } while (0);
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
                switch (curr_setting) {
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
            }
        } while (0);
        if (hKey != NULL) {
            RegCloseKey(hKey);
        }
    }
    return rv;
}

int set_its_mode(its_mode_e its_mode)
{
    int disp_version = get_dispatcher_version();
    if (disp_version >= DISPATCHER_VERSION_3) {
        int its_scmsg = ITS_SCMSG_INTELLIGENT;
        switch (its_mode) {
        case ITS_MODE_COOL:
            its_scmsg = ITS_SCMSG_BSM;
            break;
        case ITS_MODE_PERFORMANCE:
            its_scmsg = ITS_SCMSG_EPM;
            break;
        case ITS_MODE_GEEK:
            its_scmsg = ITS_SCMSG_GEEK;
            break;
        }
        return control_service(SERVICE_NAME_DISPATCHER, its_scmsg);
    } else {
        int its_scmsg = ITS_SCMSG_ENABLE;
        switch (its_mode) {
        case ITS_MODE_COOL:
            its_scmsg = ITS_SCMSG_COOL;
            break;
        case ITS_MODE_PERFORMANCE:
            its_scmsg = ITS_SCMSG_PERFORMANCE;
            break;
        case ITS_MODE_GEEK:
            its_scmsg = ITS_SCMSG_GEEK;
            break;
        }
        return control_service(SERVICE_NAME_ITS, its_scmsg);
    }
}

void run_backlight_level(backlight_level_e level) {
    /* open */
    HANDLE drv_handle = CreateFileA("\\\\.\\EnergyDrv", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (drv_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Failed to find device, make sure Lenovo energy management driver is installed!\n");
        return;
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
        backlight_level_e curr_level = BACKLIGHT_LEVEL_NONE;
        switch (status & 0x7fff) {
            case 0:
                curr_level = BACKLIGHT_LEVEL_OFF;
                break;
            case 1:
                curr_level = BACKLIGHT_LEVEL_1;
                break;
            case 2:
                curr_level = BACKLIGHT_LEVEL_2;
                break;
            case 3:
                curr_level = BACKLIGHT_LEVEL_AUTO;
                break;
        }
        printf("Current backlight level: %d\n", (int) curr_level);
        set_backlight_level(drv_handle, level);
        printf("Finished setting backlight level: %d\n", (int) level);
    } while (0);
    /* close */
    CloseHandle(drv_handle);
}

void run_charge_mode(charge_mode_e mode) {
    /* open */
    HANDLE drv_handle = CreateFileA("\\\\.\\EnergyDrv", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (drv_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Failed to find device, make sure Lenovo energy management driver is installed!\n");
        return;
    }
    /* run */
    do {
        /* get */
        charge_mode_e curr_mode = CHARGE_MODE_NONE;
        uint32_t m0 = get_charge_mode(drv_handle, 0);
        uint32_t m1 = get_charge_mode(drv_handle, 1);
        if (m0 == (uint32_t) -1 || m1 == (uint32_t) -1) {
            break;
        }
        if ((m0 & 0x04) != 0) {
            curr_mode = CHARGE_MODE_QUICK;
        } else if ((m0 & 0x20) != 0) {
            curr_mode = CHARGE_MODE_STORAGE;
        } else {
            curr_mode = CHARGE_MODE_NORMAL;
        }
        if (curr_mode == CHARGE_MODE_NORMAL) {
            if ((m1 & 0x01) != 0) {
                if ((m1 & 0x10) != 0) {
                    curr_mode = CHARGE_MODE_NIGHT;
                }
            }
        }
        printf("Current charge mode: %d\n", (int) curr_mode);
        /* set */
        set_charge_mode(drv_handle, curr_mode, mode);
        int rv = set_charge_mode_registry(mode);
        if (rv < 0) {
            fprintf(stderr, "Error: Failed to set new charge mode!\n");
            break;
        }
        printf("Finished setting charge mode: %d\n", (int) mode);
    } while (0);
    /* close */
    CloseHandle(drv_handle);
}

void run_its_mode(const char *model, its_mode_e mode) {
    do {
        its_mode_e rv = get_its_mode(model);
        if (rv == ITS_MODE_NONE) {
            fprintf(stderr, "Error: Failed to get current its mode!\n");
            break;
        }
        printf("Current its mode: %d\n", (int) rv);
        rv = set_its_mode(mode);
        if (rv < 0) {
            fprintf(stderr, "Error: Failed to set new its mode!\n");
            break;
        }
        printf("Finished setting its mode: %d\n", (int) mode);
    } while (0);
}

void usage(const char *name) {
    fprintf(stderr, "Usage: %s --backlight [0|1|2|3] --charge [0|1|2|3] --its [0|1|2|3]\n", name);
}

int main(int argc, char *argv[])
{
    char *base_name = get_base_name(argv[0]);
    if (argc != 3 && argc != 5 && argc != 7) {
        usage(base_name);
        return -1;
    }
    char sys_family[33] = { 0 };
    get_system_family(sys_family, 33);
    printf("Your model: %s\n", sys_family);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--backlight")) {
            int level = (int) strtol(argv[++i], NULL, 0);
            if (level < 0 || level > 3) {
                usage(base_name);
                return -1;
            }
            run_backlight_level((backlight_level_e) level);
        } else if (!strcmp(argv[i], "--charge")) {
            int mode = (int) strtol(argv[++i], NULL, 0);
            if (mode < 0 || mode > 3) {
                usage(base_name);
                return -1;
            }
            run_charge_mode((charge_mode_e) mode);
        } else if (!strcmp(argv[i], "--its")) {
            int mode = (int) strtol(argv[++i], NULL, 0);
            if (mode < 0 || mode > 3) {
                usage(base_name);
                return -1;
            }
            run_its_mode(sys_family, (its_mode_e) mode);
        }
    }
    return 0;
}
