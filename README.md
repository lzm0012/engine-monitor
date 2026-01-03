# Engine Monitor

A lightweight Qt Widgets simulation of an EICAS-style engine monitor without CMake. The project uses a `qmake`/`*.pro` file to target Visual Studio Qt setups.

## Features
- 5 ms timer driving synthetic engine data for two engines with start, stable, and stop phases.
- Start/Stop plus thrust increase/decrease controls that influence RPM/EGT/fuel flow.
- Sensor fault injection buttons for N1 and EGT sensors per engine plus fuel-level sensor faults.
- Automatic anomaly detection for overspeed, overtemp, fuel issues, and sensor failures with on-screen log plus rolling log files.
- CSV data logging for every sample (time, engine, N1, EGT, fuel flow, fuel level) in `logs/`.

## Building (qmake)
1. Open `engine-monitor.pro` with Qt's Visual Studio tools or run `qmake` to generate project files.
2. Build and run the generated solution; `main.cpp` launches the widget-based UI.

## Usage
- Click **Start** to begin the start-up sequence; **Stop** initiates shutdown.
- **Thrust+** / **Thrust-** adjust fuel flow and introduce small RPM/EGT changes.
- Toggle fault buttons to simulate sensor failures and review alerts in the lower log panel. Alert and data logs are written to `logs/` with a UTC timestamp in the filename.
