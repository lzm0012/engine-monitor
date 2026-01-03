#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QTextStream>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QMap>
#include <QFile>

#include "EngineSimulator.h"

class EngineWindow : public QWidget {
    Q_OBJECT
public:
    explicit EngineWindow(QWidget *parent = nullptr);
    ~EngineWindow();

private slots:
    void tick();
    void handleStart();
    void handleStop();
    void handleThrustUp();
    void handleThrustDown();
    void toggleAlertButton();

private:
    EngineSimulator m_simulator;
    QTimer m_timer;
    QFile m_dataFile;
    QFile m_logFile;
    QTextStream m_dataStream;
    QTextStream m_logStream;
    QMap<QString, QDateTime> m_lastAlert;

    QLabel *m_n1Labels[2];
    QLabel *m_egtLabels[2];
    QLabel *m_fuelFlowLabel;
    QLabel *m_fuelLevelLabel;
    QProgressBar *m_n1Bars[2];
    QProgressBar *m_egtBars[2];
    QLabel *m_startStatus;
    QLabel *m_runStatus;
    QPlainTextEdit *m_alertText;

    QPushButton *m_sensorButtons[2][2];
    QPushButton *m_sensorButtonsEgt[2][2];
    QPushButton *m_fuelLevelFaultButtons[2];

    QPushButton *createToggleButton(const QString &text);
    QLayout *buildGaugeRow(const QString &title, QLabel *valueLabel, QProgressBar *bar, const QString &minText, const QString &maxText);
    QWidget *buildStatusBox(const QString &text, QLabel **labelOut);
    void setupUi();
    void setupDataFiles();
    void appendAlert(const QString &text, const QString &severity);
    QString formatValue(double v, bool invalid) const;
    void updateIndicators();
};
