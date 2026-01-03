#pragma once

#include <QObject>
#include <QVector>
#include <QRandomGenerator>
#include <QDateTime>
#include <QString>
#include <functional>

struct SensorStatus {
    bool n1Sensor1Fault = false;
    bool n1Sensor2Fault = false;
    bool egtSensor1Fault = false;
    bool egtSensor2Fault = false;
    bool fuelLevelFault = false;
};

struct Anomaly {
    QString message;
    QString severity; // normal, caution, warning
};

struct EngineState {
    double rpm = 0.0;
    double egt = 20.0;
    double fuelFlow = 0.0;
    double fuelLevel = 20000.0;
    double ratedRpm = 40000.0;

    double startElapsed = 0.0;
    double stopElapsed = 0.0;
    bool running = false;
    bool starting = false;
    bool stopping = false;
    bool stable = false;

    SensorStatus sensors;
};

class EngineSimulator : public QObject {
    Q_OBJECT
public:
    explicit EngineSimulator(QObject *parent = nullptr);

    QVector<EngineState> states() const { return m_states; }
    const EngineState &state(int index) const { return m_states[index]; }

    void start();
    void stop();
    void increaseThrust();
    void decreaseThrust();

    void toggleN1SensorFault(int engine, int sensorIndex, bool fault);
    void toggleEgtSensorFault(int engine, int sensorIndex, bool fault);
    void toggleFuelLevelFault(int engine, bool fault);

    QVector<Anomaly> update(double dtSeconds);

signals:
    void stateChanged();

private:
    QVector<EngineState> m_states;
    double m_linearRampDuration = 2.0;

    void updateEngine(EngineState &state, double dtSeconds);
    void applyStableNoise(EngineState &state);
};
