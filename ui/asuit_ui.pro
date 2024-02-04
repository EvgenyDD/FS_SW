#-------------------------------------------------
#
# Project created by QtCreator 2020-01-29T21:23:17
#
#-------------------------------------------------

QT       += core gui
win32:QT += winextras
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += printsupport

TARGET = asuit_ui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += USE_GUI=1

CONFIG += c++1z

QMAKE_CFLAGS += -std=c++1z
win32:QMAKE_LFLAGS += -Wl,--enable-stdcall-fixup
unix:QMAKE_LFLAGS += -no-pie
QMAKE_LFLAGS += -Wl,-rpath=.


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RAPI_PATH = "$$PWD/rapi"
include($$RAPI_PATH/rapi.pri)


CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    asuit_protocol.cpp

HEADERS += \
        mainwindow.h \
    asuit_protocol.h \
    ../lib/serial_suit_protocol.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
