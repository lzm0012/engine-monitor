#include "EngineWindow.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QFrame>
#include <QSpacerItem>

EngineWindow::EngineWindow(QWidget *parent) : QWidget(parent) {
    setupUi();
    setupDataFiles();

    connect(&m_timer, &QTimer::timeout, this, &EngineWindow::tick);
    m_timer.start(5);
}

EngineWindow::~EngineWindow() {
    m_dataFile.close();
    m_logFile.close();
}

QPushButton *EngineWindow::createToggleButton(const QString &text) {
    auto *btn = new QPushButton(text);
    btn->setCheckable(true);
    connect(btn, &QPushButton::clicked, this, &EngineWindow::toggleAlertButton);
    return btn;
}

QLayout *EngineWindow::buildGaugeRow(const QString &title, QLabel *valueLabel, QProgressBar *bar, const QString &minText, const QString &maxText) {
    auto *row = new QVBoxLayout();
    auto *titleLine = new QHBoxLayout();
    auto *titleLabel = new QLabel(title);
    titleLabel->setMinimumWidth(45);
    valueLabel->setMinimumWidth(60);
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    titleLine->addWidget(titleLabel);
    titleLine->addWidget(valueLabel, 1);
    row->addLayout(titleLine);

    bar->setTextVisible(false);
    bar->setFixedHeight(14);
    auto *barLine = new QHBoxLayout();
    auto *minLabel = new QLabel(minText);
    auto *maxLabel = new QLabel(maxText);
    minLabel->setMinimumWidth(30);
    maxLabel->setMinimumWidth(30);
    barLine->addWidget(minLabel);
    barLine->addWidget(bar, 1);
    barLine->addWidget(maxLabel);
    row->addLayout(barLine);

    auto *ticks = new QHBoxLayout();
    auto *tickLeft = new QLabel(minText);
    auto *tickRight = new QLabel(maxText);
    tickLeft->setStyleSheet("color: gray;");
    tickRight->setStyleSheet("color: gray;");
    ticks->addWidget(tickLeft);
    ticks->addStretch();
    ticks->addWidget(tickRight);
    row->addLayout(ticks);
    return row;
}

QWidget *EngineWindow::buildStatusBox(const QString &text, QLabel **labelOut) {
    auto *box = new QFrame();
    box->setFrameShape(QFrame::Box);
    box->setLineWidth(1);
    auto *layout = new QHBoxLayout(box);
    layout->setContentsMargins(6, 4, 6, 4);
    auto *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setAutoFillBackground(true);
    layout->addWidget(label);
    *labelOut = label;
    return box;
}

void EngineWindow::setupUi() {
    setWindowTitle(tr("Virtual Engine Monitor"));
    auto *layout = new QVBoxLayout(this);

    auto *gauges = new QGridLayout();
    gauges->setHorizontalSpacing(18);
    gauges->setVerticalSpacing(12);

    for (int i = 0; i < 2; ++i) {
        m_n1Labels[i] = new QLabel("0.0");
        m_egtLabels[i] = new QLabel("0.0");
        m_n1Bars[i] = new QProgressBar();
        m_n1Bars[i]->setRange(0, 125);
        m_egtBars[i] = new QProgressBar();
        m_egtBars[i]->setRange(-5, 1200);

        gauges->addLayout(buildGaugeRow(QString("N1 %1").arg(i + 1), m_n1Labels[i], m_n1Bars[i], "0%", "100%"), 0, i);
        gauges->addLayout(buildGaugeRow(QString("EGT %1").arg(i + 1), m_egtLabels[i], m_egtBars[i], "0", "1200"), 1, i);
    }
    layout->addLayout(gauges);

    m_fuelFlowLabel = new QLabel("0.0");
    m_fuelLevelLabel = new QLabel("20000");
    auto *fuelRow = new QHBoxLayout();
    fuelRow->setSpacing(12);
    fuelRow->addWidget(new QLabel(tr("Fuel")));
    fuelRow->addStretch();
    fuelRow->addWidget(new QLabel(tr("Flow")));
    fuelRow->addWidget(m_fuelFlowLabel);
    fuelRow->addStretch();
    fuelRow->addWidget(new QLabel(tr("Level")));
    fuelRow->addWidget(m_fuelLevelLabel);
    layout->addLayout(fuelRow);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    auto *startBtn = new QPushButton(tr("START"));
    auto *stopBtn = new QPushButton(tr("STOP"));
    auto *thrustUp = new QPushButton(tr("Thrust+"));
    auto *thrustDown = new QPushButton(tr("Thrust-"));
    connect(startBtn, &QPushButton::clicked, this, &EngineWindow::handleStart);
    connect(stopBtn, &QPushButton::clicked, this, &EngineWindow::handleStop);
    connect(thrustUp, &QPushButton::clicked, this, &EngineWindow::handleThrustUp);
    connect(thrustDown, &QPushButton::clicked, this, &EngineWindow::handleThrustDown);
    btnLayout->addWidget(new QLabel(tr("Start")));
    btnLayout->addWidget(new QLabel(tr("Stop")));
    btnLayout->addStretch();
    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(thrustUp);
    btnLayout->addWidget(thrustDown);
    layout->addLayout(btnLayout);

    auto *statusRow = new QHBoxLayout();
    statusRow->setSpacing(12);
    statusRow->addWidget(buildStatusBox(tr("START"), &m_startStatus));
    statusRow->addWidget(buildStatusBox(tr("RUN"), &m_runStatus));
    statusRow->addStretch();
    layout->addLayout(statusRow);

    auto *faultBox = new QGroupBox(tr("Sensor Fault Injection"));
    auto *faultLayout = new QGridLayout();
    faultLayout->setHorizontalSpacing(10);
    faultLayout->setVerticalSpacing(6);
    for (int eng = 0; eng < 2; ++eng) {
        for (int sensor = 0; sensor < 2; ++sensor) {
            m_sensorButtons[eng][sensor] = createToggleButton(QString("N1E%1S%2").arg(eng + 1).arg(sensor + 1));
            m_sensorButtonsEgt[eng][sensor] = createToggleButton(QString("EGTE%1S%2").arg(eng + 1).arg(sensor + 1));
            faultLayout->addWidget(m_sensorButtons[eng][sensor], eng * 2, sensor);
            faultLayout->addWidget(m_sensorButtonsEgt[eng][sensor], eng * 2 + 1, sensor);
        }
        m_fuelLevelFaultButtons[eng] = createToggleButton(QString("FuelE%1").arg(eng + 1));
        faultLayout->addWidget(m_fuelLevelFaultButtons[eng], eng * 2, 2);
    }
    faultBox->setLayout(faultLayout);
    layout->addWidget(faultBox);

    m_alertText = new QPlainTextEdit();
    m_alertText->setReadOnly(true);
    m_alertText->setMaximumHeight(120);
    layout->addWidget(m_alertText);

    setLayout(layout);
}

void EngineWindow::setupDataFiles() {
    QString stamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss");
    QDir().mkpath("logs");
    m_dataFile.setFileName(QString("logs/data_%1.csv").arg(stamp));
    m_logFile.setFileName(QString("logs/alert_%1.log").arg(stamp));
    if (m_dataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_dataStream.setDevice(&m_dataFile);
        m_dataStream << "time_ms,engine,n1,egt,fuel_flow,fuel_level\n";
    }
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
    }
}

void EngineWindow::appendAlert(const QString &text, const QString &severity) {
    QString line = QString("[%1][%2] %3")
                       .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                       .arg(severity.toUpper())
                       .arg(text);
    m_alertText->appendPlainText(line);

    auto now = QDateTime::currentDateTime();
    if (!m_lastAlert.contains(text) || m_lastAlert[text].msecsTo(now) > 5000) {
        m_lastAlert[text] = now;
        if (m_logStream.device()) {
            m_logStream << now.toString(Qt::ISODate) << "," << text << "\n";
            m_logStream.flush();
        }
    }
}

QString EngineWindow::formatValue(double v, bool invalid) const {
    if (invalid)
        return "--";
    return QString::number(v, 'f', 1);
}

void EngineWindow::updateIndicators() {
    auto states = m_simulator.states();
    for (int i = 0; i < states.size(); ++i) {
        bool n1Invalid = states[i].sensors.n1Sensor1Fault && states[i].sensors.n1Sensor2Fault;
        bool egtInvalid = states[i].sensors.egtSensor1Fault && states[i].sensors.egtSensor2Fault;
        double n1Percent = (states[i].ratedRpm == 0.0) ? 0.0 : states[i].rpm / states[i].ratedRpm * 100.0;
        m_n1Labels[i]->setText(formatValue(n1Percent, n1Invalid));
        m_egtLabels[i]->setText(formatValue(states[i].egt, egtInvalid));
        m_n1Bars[i]->setValue(static_cast<int>(n1Percent));
        m_egtBars[i]->setValue(static_cast<int>(states[i].egt));
    }
    m_fuelFlowLabel->setText(QString::number(states.first().fuelFlow, 'f', 1));
    m_fuelLevelLabel->setText(QString::number(states.first().fuelLevel, 'f', 0));

    QPalette startPal = m_startStatus->palette();
    QPalette runPal = m_runStatus->palette();
    QColor startColor = states.first().starting ? QColor("#6fcf6f") : QColor("#dddddd");
    QColor runColor = (states.first().stable && states.first().rpm / states.first().ratedRpm * 100.0 >= 95.0) ? QColor("#6fcf6f") : QColor("#dddddd");
    startPal.setColor(QPalette::Window, startColor);
    startPal.setColor(QPalette::WindowText, Qt::black);
    runPal.setColor(QPalette::Window, runColor);
    runPal.setColor(QPalette::WindowText, Qt::black);
    m_startStatus->setPalette(startPal);
    m_runStatus->setPalette(runPal);
}

void EngineWindow::tick() {
    QVector<Anomaly> alerts = m_simulator.update(0.005);
    updateIndicators();

    auto states = m_simulator.states();
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < states.size(); ++i) {
        if (m_dataStream.device()) {
            m_dataStream << now << "," << i << "," << states[i].rpm << "," << states[i].egt << "," << states[i].fuelFlow << "," << states[i].fuelLevel << "\n";
        }
    }
    if (m_dataStream.device()) {
        m_dataStream.flush();
    }

    for (const auto &a : alerts) {
        appendAlert(a.message, a.severity);
    }
}

void EngineWindow::handleStart() {
    m_simulator.start();
}

void EngineWindow::handleStop() {
    m_simulator.stop();
}

void EngineWindow::handleThrustUp() {
    m_simulator.increaseThrust();
}

void EngineWindow::handleThrustDown() {
    m_simulator.decreaseThrust();
}

void EngineWindow::toggleAlertButton() {
    QObject *s = sender();
    for (int eng = 0; eng < 2; ++eng) {
        for (int sensor = 0; sensor < 2; ++sensor) {
            if (s == m_sensorButtons[eng][sensor]) {
                m_simulator.toggleN1SensorFault(eng, sensor, m_sensorButtons[eng][sensor]->isChecked());
            }
            if (s == m_sensorButtonsEgt[eng][sensor]) {
                m_simulator.toggleEgtSensorFault(eng, sensor, m_sensorButtonsEgt[eng][sensor]->isChecked());
            }
        }
        if (s == m_fuelLevelFaultButtons[eng]) {
            m_simulator.toggleFuelLevelFault(eng, m_fuelLevelFaultButtons[eng]->isChecked());
        }
    }
}
