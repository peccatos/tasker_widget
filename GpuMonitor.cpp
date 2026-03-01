#include "GpuMonitor.h"
#include <QDebug>

// ========================================================================
// ВНИМАНИЕ: Чтобы включить реальный опрос GPU, раскомментируйте нужный define
// ПОСЛЕ того, как скачаете и положите SDK в папку проекта.
// ========================================================================
#define ENABLE_NVAPI
// #define ENABLE_AMD_ADL

#ifdef ENABLE_NVAPI
#include "nvapi.h"
#endif

#ifdef ENABLE_AMD_ADL
#include "adl_sdk.h"
#include <windows.h>

#ifndef ADL_OK
#define ADL_OK 0
#endif

// Типы функций для динамической загрузки AMD ADL (atiadlxx.dll)
typedef void* (__stdcall *ADL_MAIN_MALLOC_CALLBACK)(int);
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)();
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
typedef int (*ADL_OVERDRIVE5_TEMPERATURE_GET)(int, int, ADLTemperature*);
typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int, ADLPMActivity*);
typedef int (*ADL_ADAPTER_MEMORYINFO_GET)(int, ADLMemoryInfo*);

// Указатели на функции
HINSTANCE hDLL = nullptr;
ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = nullptr;
ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy = nullptr;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = nullptr;
ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get = nullptr;
ADL_OVERDRIVE5_TEMPERATURE_GET ADL_Overdrive5_Temperature_Get = nullptr;
ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get = nullptr;
ADL_ADAPTER_MEMORYINFO_GET ADL_Adapter_MemoryInfo_Get = nullptr;

// Функция выделения памяти для ADL
void* __stdcall ADL_Main_Memory_Alloc(int iSize) {
    void* lpBuffer = malloc(iSize);
    return lpBuffer;
}

int amdAdapterIndex = -1;
#endif

GpuMonitor::GpuMonitor() : hasNvidia(false), hasAmd(false) {
}

GpuMonitor::~GpuMonitor() {
#ifdef ENABLE_AMD_ADL
    if (hasAmd && ADL_Main_Control_Destroy) {
        ADL_Main_Control_Destroy();
    }
    if (hDLL) {
        FreeLibrary(hDLL);
    }
#endif
}

bool GpuMonitor::init() {
    // Пытаемся инициализировать NVIDIA
    if (initNvidia()) {
        hasNvidia = true;
        return true;
    }

    // Если не NVIDIA, пробуем AMD
    if (initAmd()) {
        hasAmd = true;
        return true;
    }

    return false; // Дискретная видеокарта не найдена или SDK не подключен
}

GpuStats GpuMonitor::getStats() {
    if (hasNvidia) return getNvidiaStats();
    if (hasAmd) return getAmdStats();

    // Возвращаем пустую структуру, если GPU нет
    return GpuStats();
}

// ---------------------------------------------------------
// NVIDIA (NVAPI) Реализация
// ---------------------------------------------------------
bool GpuMonitor::initNvidia() {
#ifdef ENABLE_NVAPI
    NvAPI_Status status = NvAPI_Initialize();
    return status == NVAPI_OK;
#else
    return false;
#endif
}

GpuStats GpuMonitor::getNvidiaStats() {
    GpuStats stats;
#ifdef ENABLE_NVAPI
    NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
    NvU32 gpuCount = 0;

    if (NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount) == NVAPI_OK && gpuCount > 0) {
        stats.isAvailable = true;

        // 1. Имя GPU
        NvAPI_ShortString name;
        if (NvAPI_GPU_GetFullName(gpuHandles[0], name) == NVAPI_OK) {
            stats.name = QString::fromLocal8Bit(name);
        }

        // 2. Загрузка GPU (Load)
        NV_GPU_DYNAMIC_PSTATES_INFO_EX pstatesInfo;
        pstatesInfo.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
        if (NvAPI_GPU_GetDynamicPstatesInfoEx(gpuHandles[0], &pstatesInfo) == NVAPI_OK) {
            stats.loadPercent = pstatesInfo.utilization[0].percentage;
        }

        // 3. Температура
        NV_GPU_THERMAL_SETTINGS thermal;
        thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
        if (NvAPI_GPU_GetThermalSettings(gpuHandles[0], NVAPI_THERMAL_TARGET_ALL, &thermal) == NVAPI_OK) {
            stats.temperatureC = thermal.sensor[0].currentTemp;
        }

        // 4. VRAM (Память)
        NV_DISPLAY_DRIVER_MEMORY_INFO memInfo;
        memInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;
        if (NvAPI_GPU_GetMemoryInfo(gpuHandles[0], &memInfo) == NVAPI_OK) {
            stats.vramTotalGB = memInfo.dedicatedVideoMemory / 1024.0 / 1024.0;
            stats.vramUsedGB = (memInfo.dedicatedVideoMemory - memInfo.curAvailableDedicatedVideoMemory) / 1024.0 / 1024.0;
        }
    }
#endif
    return stats;
}

// ---------------------------------------------------------
// AMD (ADL) Реализация
// ---------------------------------------------------------
bool GpuMonitor::initAmd() {
#ifdef ENABLE_AMD_ADL
    hDLL = LoadLibrary(TEXT("atiadlxx.dll"));
    if (hDLL == nullptr) {
        hDLL = LoadLibrary(TEXT("atiadlxy.dll")); // 32-bit fallback on 64-bit OS
    }

    if (hDLL == nullptr) return false;

    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL_Main_Control_Create");
    ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(hDLL, "ADL_Main_Control_Destroy");
    ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(hDLL, "ADL_Adapter_NumberOfAdapters_Get");
    ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hDLL, "ADL_Adapter_AdapterInfo_Get");
    ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET)GetProcAddress(hDLL, "ADL_Overdrive5_Temperature_Get");
    ADL_Overdrive5_CurrentActivity_Get = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(hDLL, "ADL_Overdrive5_CurrentActivity_Get");
    ADL_Adapter_MemoryInfo_Get = (ADL_ADAPTER_MEMORYINFO_GET)GetProcAddress(hDLL, "ADL_Adapter_MemoryInfo_Get");

    if (!ADL_Main_Control_Create || !ADL_Adapter_NumberOfAdapters_Get || !ADL_Adapter_AdapterInfo_Get) {
        return false;
    }

    if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) != ADL_OK) {
        return false;
    }

    int numAdapters = 0;
    if (ADL_Adapter_NumberOfAdapters_Get(&numAdapters) != ADL_OK || numAdapters == 0) {
        return false;
    }

    LPAdapterInfo adapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * numAdapters);
    memset(adapterInfo, '\0', sizeof(AdapterInfo) * numAdapters);
    ADL_Adapter_AdapterInfo_Get(adapterInfo, sizeof(AdapterInfo) * numAdapters);

    // Ищем первый активный AMD адаптер
    for (int i = 0; i < numAdapters; i++) {
        if (adapterInfo[i].iVendorID == 1002) { // 1002 is AMD/ATI Vendor ID
            amdAdapterIndex = adapterInfo[i].iAdapterIndex;
            break;
        }
    }

    free(adapterInfo);
    return amdAdapterIndex != -1;
#else
    return false;
#endif
}

GpuStats GpuMonitor::getAmdStats() {
    GpuStats stats;
#ifdef ENABLE_AMD_ADL
    if (amdAdapterIndex != -1) {
        stats.isAvailable = true;
        stats.name = "AMD Radeon Graphics"; // Можно получить точное имя из AdapterInfo

        // Температура
        if (ADL_Overdrive5_Temperature_Get) {
            ADLTemperature temp = {0};
            temp.iSize = sizeof(ADLTemperature);
            if (ADL_Overdrive5_Temperature_Get(amdAdapterIndex, 0, &temp) == ADL_OK) {
                stats.temperatureC = temp.iTemperature / 1000; // ADL возвращает в миллиградусах
            }
        }

        // Нагрузка (Load)
        if (ADL_Overdrive5_CurrentActivity_Get) {
            ADLPMActivity activity = {0};
            activity.iSize = sizeof(ADLPMActivity);
            if (ADL_Overdrive5_CurrentActivity_Get(amdAdapterIndex, &activity) == ADL_OK) {
                stats.loadPercent = activity.iEngineClock > 0 ? activity.iActivityPercent : 0;
            }
        }

        // VRAM
        if (ADL_Adapter_MemoryInfo_Get) {
            ADLMemoryInfo memInfo = {0};
            if (ADL_Adapter_MemoryInfo_Get(amdAdapterIndex, &memInfo) == ADL_OK) {
                stats.vramTotalGB = memInfo.iMemorySize / 1024.0 / 1024.0;
                // ADL не всегда легко отдает Used VRAM, используем заглушку или расчет
                stats.vramUsedGB = stats.vramTotalGB * (stats.loadPercent / 100.0); // Упрощенная эмуляция
            }
        }
    }
#endif
    return stats;
}
