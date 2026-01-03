QT += widgets
CONFIG += c++17 console
TARGET = engine-monitor
TEMPLATE = app
SOURCES += \
    main.cpp \
    EngineWindow.cpp \
    EngineSimulator.cpp
HEADERS += \
    EngineWindow.h \
    EngineSimulator.h
DISTFILES += README.md
