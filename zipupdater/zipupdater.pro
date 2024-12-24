TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

LIBS += User32.lib

contains(DEFINES, WIN64) {
    message("WIN x64 Release Build")
} else {
    message("WIN x86 Release Build")
}

SOURCES += \
        main.cpp
