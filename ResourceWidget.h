#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPoint>
#include "GpuMonitor.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class ResourceWidget : public QWidget {
    Q_OBJECT

public:
    ResourceWidget(QWidget *parent = nullptr);
    ~ResourceWidget();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void toggleState();
    void openTaskManager();
    void openVPN();
    void updateSystemStats();
    void closeApp();

private:
    void setupUI();
    QIcon createSvgIcon(const QString &svgString);
    void loadConfig();

    // Состояние виджета
    bool isExpanded;
    bool hasGPU;
    QPoint dragPosition;
    QString vpnPath; // Путь к VPN из конфига

    // Монитор GPU
    GpuMonitor gpuMonitor;

    // Элементы UI
    QPushButton *btnMinimize;
    QWidget *statsContainer;
    QLabel *cpuValueLabel;
    QLabel *ramValueLabel;
    QLabel *gpuValueLabel;
    QLabel *vramValueLabel;
    QWidget *gpuWidget; // Контейнер GPU для скрытия, если GPU нет

    QTimer *updateTimer;

#ifdef Q_OS_WIN
    // Для расчета CPU
    ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    int numProcessors;
    HANDLE self;
    void initCPUTracking();
    double getCPULoad();
#endif
};
