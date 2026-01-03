#include "EngineSimulator.h"

#include <QtMath>

EngineSimulator::EngineSimulator(QObject *parent) : QObject(parent) {
    m_states.resize(2);
}

void EngineSimulator::start() {
    for (auto &state : m_states) {
        state.startElapsed = 0.0;
        state.stopElapsed = 0.0;
        state.starting = true;
        state.stopping = false;
        state.stable = false;
        state.running = true;
    }
}

void EngineSimulator::stop() {
    for (auto &state : m_states) {
        state.stopping = true;
        state.starting = false;
        state.stable = false;
        state.stopElapsed = 0.0;
    }
}

void EngineSimulator::increaseThrust() {
    for (auto &state : m_states) {
        state.fuelFlow = qMin(50.0, state.fuelFlow + 1.0);
        state.rpm *= 1.03 + QRandomGenerator::global()->bounded(0.02);
        state.egt *= 1.03 + QRandomGenerator::global()->bounded(0.02);
    }
}

void EngineSimulator::decreaseThrust() {
    for (auto &state : m_states) {
        state.fuelFlow = qMax(0.0, state.fuelFlow - 1.0);
        state.rpm *= 0.97 - QRandomGenerator::global()->bounded(0.02);
        state.egt *= 0.97 - QRandomGenerator::global()->bounded(0.02);
    }
}

void EngineSimulator::toggleN1SensorFault(int engine, int sensorIndex, bool fault) {
    if (engine < 0 || engine >= m_states.size()) return;
    if (sensorIndex == 0)
        m_states[engine].sensors.n1Sensor1Fault = fault;
    else
        m_states[engine].sensors.n1Sensor2Fault = fault;
}

void EngineSimulator::toggleEgtSensorFault(int engine, int sensorIndex, bool fault) {
    if (engine < 0 || engine >= m_states.size()) return;
    if (sensorIndex == 0)
        m_states[engine].sensors.egtSensor1Fault = fault;
    else
        m_states[engine].sensors.egtSensor2Fault = fault;
}

void EngineSimulator::toggleFuelLevelFault(int engine, bool fault) {
    if (engine < 0 || engine >= m_states.size()) return;
    m_states[engine].sensors.fuelLevelFault = fault;
}

void EngineSimulator::applyStableNoise(EngineState &state) {
    double noise = (QRandomGenerator::global()->bounded(0.06) - 0.03);
    state.rpm = state.ratedRpm * qBound(0.95, 0.98 + noise, 1.03);
    state.egt = qBound(20.0, 850.0 + noise * 100.0, 1100.0);
    state.fuelFlow = qBound(5.0, 30.0 + noise * 5.0, 40.0);
}

void EngineSimulator::updateEngine(EngineState &state, double dtSeconds) {
    const double startFuelRamp = 5.0; // units per second for first 2s
    const double rpmLinearRate = 10400.0; // rpm per second
    const double rated = state.ratedRpm;

    if (state.starting) {
        state.startElapsed += dtSeconds;
        if (state.startElapsed <= m_linearRampDuration) {
            state.rpm = qMin(rated * 0.6, state.rpm + rpmLinearRate * dtSeconds);
            state.fuelFlow = qMin(20.0, state.fuelFlow + startFuelRamp * dtSeconds);
            state.egt = qMin(600.0, state.egt + 50.0 * dtSeconds);
        } else {
            double t = state.startElapsed;
            state.fuelFlow = qMin(42.0 * log10(qMax(1.1, t - 1.0)) + 10.0 + QRandomGenerator::global()->bounded(1.0), 45.0);
            state.rpm = 23000.0 * log10(qMax(1.1, t - 1.0)) + 20000.0 + QRandomGenerator::global()->bounded(500.0);
            state.egt = 900.0 * log10(qMax(1.1, t - 1.0)) + 20.0 + QRandomGenerator::global()->bounded(20.0);
            state.rpm = qMin(rated * 0.95, state.rpm);
            state.egt = qMin(1000.0, state.egt);
        }
        if (state.rpm >= rated * 0.95) {
            state.starting = false;
            state.stable = true;
        }
    } else if (state.stable && !state.stopping) {
        applyStableNoise(state);
    }

    if (state.stopping) {
        state.stopElapsed += dtSeconds;
        double decay = pow(0.85, dtSeconds * 2.0);
        state.rpm = state.rpm * decay;
        double egtDecay = pow(0.9, dtSeconds * 2.0);
        state.egt = qMax(20.0, state.egt * egtDecay);
        state.fuelFlow = 0.0;
        if (state.stopElapsed >= 10.0 || state.rpm < 10.0) {
            state.running = false;
            state.stopping = false;
            state.rpm = 0.0;
            state.egt = 20.0;
        }
    }

    double consumption = state.fuelFlow * dtSeconds;
    state.fuelLevel = qMax(0.0, state.fuelLevel - consumption);
}

QVector<Anomaly> EngineSimulator::update(double dtSeconds) {
    QVector<Anomaly> alerts;
    for (auto &state : m_states) {
        updateEngine(state, dtSeconds);

        double n1Percent = (state.ratedRpm == 0.0) ? 0.0 : state.rpm / state.ratedRpm * 100.0;
        bool startingPhase = state.starting;
        bool stablePhase = state.stable && !state.stopping;

        if (state.sensors.n1Sensor1Fault && state.sensors.n1Sensor2Fault) {
            alerts.push_back({"Both N1 sensors failed - engine shutdown", "warning"});
            state.stopping = true;
        } else if (state.sensors.n1Sensor1Fault || state.sensors.n1Sensor2Fault) {
            alerts.push_back({"N1 sensor fault detected", "caution"});
        }

        if (state.sensors.egtSensor1Fault && state.sensors.egtSensor2Fault) {
            alerts.push_back({"Both EGT sensors failed - engine shutdown", "warning"});
            state.stopping = true;
        } else if (state.sensors.egtSensor1Fault || state.sensors.egtSensor2Fault) {
            alerts.push_back({"EGT sensor fault detected", "caution"});
        }

        if (state.sensors.fuelLevelFault) {
            alerts.push_back({"Fuel level sensor invalid", "warning"});
        } else if (state.fuelLevel < 1000.0) {
            alerts.push_back({"Low fuel level", "caution"});
        }

        if (state.fuelFlow > 50.0) {
            alerts.push_back({"Fuel flow high", "caution"});
        }

        if (n1Percent > 120.0) {
            alerts.push_back({"N1 overspeed 2 - shutdown", "warning"});
            state.stopping = true;
        } else if (n1Percent > 105.0) {
            alerts.push_back({"N1 overspeed 1", "caution"});
        }

        if (startingPhase) {
            if (state.egt > 1000.0) {
                alerts.push_back({"Start overtemp 2 - shutdown", "warning"});
                state.stopping = true;
            } else if (state.egt > 850.0) {
                alerts.push_back({"Start overtemp 1", "caution"});
            }
        }

        if (stablePhase) {
            if (state.egt > 1100.0) {
                alerts.push_back({"Run overtemp 2 - shutdown", "warning"});
                state.stopping = true;
            } else if (state.egt > 950.0) {
                alerts.push_back({"Run overtemp 1", "caution"});
            }
        }
    }

    emit stateChanged();
    return alerts;
}
