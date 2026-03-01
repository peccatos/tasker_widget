#include "ResourceWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QMouseEvent>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>

// Вшитые SVG иконки (Lucide React)
const QString iconVPN = R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#b4b4b4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="7.5" cy="15.5" r="5.5"/><path d="m21 2-9.6 9.6"/><path d="m15.5 7.5 2.3 2.3a1 1 0 0 0 1.4 0l2.1-2.1a1 1 0 0 0 0-1.4L19 4"/></svg>)";
const QString iconTaskMgr = R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#b4b4b4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.48 12H2"/></svg>)";
const QString iconCollapse = R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#b4b4b4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m18 15-6-6-6 6"/></svg>)";
const QString iconExpand = R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#b4b4b4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"/></svg>)";
const QString iconClose = R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#b4b4b4" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 6 6 18"/><path d="m6 6 12 12"/></svg>)";

ResourceWidget::ResourceWidget(QWidget *parent)
    : QWidget(parent), isExpanded(true), hasGPU(true), vpnPath("ms-settings:network-vpn") {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    loadConfig();

    // Инициализация GPU монитора
    hasGPU = gpuMonitor.init();

    setupUI();

#ifdef Q_OS_WIN
    initCPUTracking();
#endif

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &ResourceWidget::updateSystemStats);
    updateTimer->start(1000);
    updateSystemStats();
}

ResourceWidget::~ResourceWidget() {}

void ResourceWidget::loadConfig() {
    QFile configFile("config.json");
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(data));
        QJsonObject json = doc.object();
        if (json.contains("vpn_path")) {
            vpnPath = json["vpn_path"].toString();
        }
        // Если в конфиге жестко задано скрыть GPU
        if (json.contains("force_hide_gpu") && json["force_hide_gpu"].toBool()) {
            hasGPU = false;
        }
    }
}

QIcon ResourceWidget::createSvgIcon(const QString &svgString) {
    QSvgRenderer renderer(svgString.toUtf8());
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return QIcon(pixmap);
}

void ResourceWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize); // Гарантирует сжатие окна при скрытии элементов

    QWidget *bgWidget = new QWidget(this);
    bgWidget->setObjectName("bgWidget");
    bgWidget->setStyleSheet("#bgWidget { background-color: white; border-radius: 16px; border: 1px solid #e5e5e5; }");

    QVBoxLayout *bgLayout = new QVBoxLayout(bgWidget);
    bgLayout->setContentsMargins(16, 12, 16, 16);
    bgLayout->setSpacing(12);

    // --- ВЕРХНЯЯ ПАНЕЛЬ ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();

    QPushButton *btnVPN = new QPushButton(createSvgIcon(iconVPN), "", this);
    QPushButton *btnTaskMgr = new QPushButton(createSvgIcon(iconTaskMgr), "", this);
    btnMinimize = new QPushButton(createSvgIcon(iconCollapse), "", this);
    QPushButton *btnClose = new QPushButton(createSvgIcon(iconClose), "", this);

    QString btnStyle = "QPushButton { border: none; background: transparent; padding: 4px; } QPushButton:hover { background: #f0f0f0; border-radius: 4px; }";
    QString btnCloseStyle = "QPushButton { border: none; background: transparent; padding: 4px; } QPushButton:hover { background: #fee2e2; border-radius: 4px; }";

    btnVPN->setStyleSheet(btnStyle);
    btnTaskMgr->setStyleSheet(btnStyle);
    btnMinimize->setStyleSheet(btnStyle);
    btnClose->setStyleSheet(btnCloseStyle);

    btnVPN->setCursor(Qt::PointingHandCursor);
    btnTaskMgr->setCursor(Qt::PointingHandCursor);
    btnMinimize->setCursor(Qt::PointingHandCursor);
    btnClose->setCursor(Qt::PointingHandCursor);

    connect(btnVPN, &QPushButton::clicked, this, &ResourceWidget::openVPN);
    connect(btnTaskMgr, &QPushButton::clicked, this, &ResourceWidget::openTaskManager);
    connect(btnMinimize, &QPushButton::clicked, this, &ResourceWidget::toggleState);
    connect(btnClose, &QPushButton::clicked, this, &ResourceWidget::closeApp);

    topLayout->addWidget(btnVPN);
    topLayout->addWidget(btnTaskMgr);
    topLayout->addWidget(btnMinimize);

    QFrame *vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);
    vLine->setStyleSheet("color: #e5e5e5; background-color: #e5e5e5; width: 1px; margin-top: 4px; margin-bottom: 4px;");
    topLayout->addWidget(vLine);

    topLayout->addWidget(btnClose);

    // --- НИЖНЯЯ ПАНЕЛЬ ---
    statsContainer = new QWidget(this);
    QHBoxLayout *statsLayout = new QHBoxLayout(statsContainer);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(8);

    QString pillStyle = "background-color: #f5f5f5; border-radius: 14px; color: #a3a3a3; font-weight: bold; font-size: 12px;";

    // GPU & VRAM
    gpuWidget = new QWidget();
    gpuWidget->setStyleSheet(pillStyle);
    QHBoxLayout *gpuLayout = new QHBoxLayout(gpuWidget);
    gpuLayout->setContentsMargins(16, 6, 16, 6);

    gpuValueLabel = new QLabel("0%  0°C");
    vramValueLabel = new QLabel("0.00/0.00 GB");
    vramValueLabel->setMinimumWidth(100);

    gpuLayout->addWidget(new QLabel("GPU"));
    gpuLayout->addWidget(gpuValueLabel);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setStyleSheet("color: white; background-color: white; width: 2px;");
    gpuLayout->addWidget(line);

    gpuLayout->addWidget(new QLabel("VRAM"));
    gpuLayout->addWidget(vramValueLabel);

    if (!hasGPU) {
        gpuWidget->hide();
    }

    // CPU
    QWidget *cpuWidget = new QWidget();
    cpuWidget->setStyleSheet(pillStyle);
    QHBoxLayout *cpuLayout = new QHBoxLayout(cpuWidget);
    cpuLayout->setContentsMargins(16, 6, 16, 6);
    cpuValueLabel = new QLabel("0%");
    cpuLayout->addWidget(new QLabel("CPU"));
    cpuLayout->addStretch();
    cpuLayout->addWidget(cpuValueLabel);

    // RAM
    QWidget *ramWidget = new QWidget();
    ramWidget->setStyleSheet(pillStyle);
    QHBoxLayout *ramLayout = new QHBoxLayout(ramWidget);
    ramLayout->setContentsMargins(16, 6, 16, 6);
    ramValueLabel = new QLabel("0%");
    ramLayout->addWidget(new QLabel("ОЗУ"));
    ramLayout->addStretch();
    ramLayout->addWidget(ramValueLabel);

    statsLayout->addWidget(gpuWidget);
    statsLayout->addWidget(cpuWidget);
    statsLayout->addWidget(ramWidget);

    bgLayout->addLayout(topLayout);
    bgLayout->addWidget(statsContainer);
    mainLayout->addWidget(bgWidget);
}

void ResourceWidget::toggleState() {
    isExpanded = !isExpanded;
    statsContainer->setVisible(isExpanded);
    btnMinimize->setIcon(createSvgIcon(isExpanded ? iconCollapse : iconExpand));
    adjustSize();
}

void ResourceWidget::closeApp() {
    QApplication::quit();
}

void ResourceWidget::openTaskManager() {
    QProcess::startDetached("taskmgr.exe");
}

void ResourceWidget::openVPN() {
    if (vpnPath == "ms-settings:network-vpn") {
        QProcess::startDetached("explorer.exe", {vpnPath});
    } else {
        QProcess::startDetached(vpnPath);
    }
}

void ResourceWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void ResourceWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

#ifdef Q_OS_WIN
void ResourceWidget::initCPUTracking() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

double ResourceWidget::getCPULoad() {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    double percent;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;

    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    return percent * 100.0;
}
#endif

void ResourceWidget::updateSystemStats() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORD ramUsage = memInfo.dwMemoryLoad;
    ramValueLabel->setText(QString::number(ramUsage) + "%");

    double cpuUsage = getCPULoad();
    cpuValueLabel->setText(QString::number((int)(cpuUsage * 10)) + "%");

    // Обновление GPU (если доступно)
    if (hasGPU) {
        GpuStats stats = gpuMonitor.getStats();
        if (stats.isAvailable) {
            gpuValueLabel->setText(QString::number(stats.loadPercent) + "%  " + QString::number(stats.temperatureC) + "°C");
            vramValueLabel->setText(QString::number(stats.vramUsedGB, 'f', 2) + "/" + QString::number(stats.vramTotalGB, 'f', 2) + " GB");
        }
    }
#endif
}
