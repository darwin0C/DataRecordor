QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

UI_DIR  = ./obj/Gui
MOC_DIR = ./obj/Moc
OBJECTS_DIR = ./obj/Obj
DESTDIR=./bin
TARGET=DataRecordor

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    data.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

#DEFINES += LINUX_MODE

#DEFINES += TEST_MODE

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include($$PWD/Com/ComManager.pri)
include($$PWD/Function/Function.pri)
include($$PWD/CanEdit/CanEdit.pri)
include($$PWD/DataBase/DataBase.pri)
include($$PWD/FileSave/FileSave.pri)
include($$PWD/Settings/Settings.pri)
