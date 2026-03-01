#pragma once

#include <QString>

// Структура для хранения данных о видеокарте
struct GpuStats {
    bool isAvailable = false;
    QString name = "Unknown GPU";
    int loadPercent = 0;
    int temperatureC = 0;
    double vramUsedGB = 0.0;
    double vramTotalGB = 0.0;
};

class GpuMonitor {
public:
    GpuMonitor();
    ~GpuMonitor();

    // Инициализация SDK (определяет, какая видеокарта установлена)
    bool init();

    // Получение текущих метрик
    GpuStats getStats();

private:
    bool hasNvidia;
    bool hasAmd;

    bool initNvidia();
    bool initAmd();

    GpuStats getNvidiaStats();
    GpuStats getAmdStats();
};
