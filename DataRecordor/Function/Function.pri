QT       += core  network sql xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 debug

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/CommandCtrol.h \
    $$PWD/CommandData.h \
    $$PWD/DeviceData.h \
    $$PWD/DeviceManager.h \
    $$PWD/EventInfo.h \
    $$PWD/devicestat.h

SOURCES += \
    $$PWD/CommandCtrol.cpp \
    $$PWD/DeviceManager.cpp \
    $$PWD/EventInfo.cpp \
    $$PWD/devicestat.cpp


