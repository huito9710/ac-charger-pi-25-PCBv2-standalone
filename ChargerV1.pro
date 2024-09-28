#-------------------------------------------------
#
# Project created by QtCreator 2016-05-11T14:59:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ChargerV1
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    chargingwindow.cpp \
    videoplay.cpp \
    cablewin.cpp \
    paymentmethodwindow.cpp \
    messagewindow.cpp \
    octopuswindow.cpp \
    wapwindow.cpp \
    chargefinishwindow.cpp \
    OCPP/ocppclient.cpp \
    OCPP/cocppthread.cpp \
    api_function.cpp \
    ccharger.cpp \
    ccomportthread.cpp \
    database.cpp \
    crfidthread.cpp \
    sockettype.cpp

HEADERS  += mainwindow.h \
    chargingwindow.h \
    const.h \
    videoplay.h \
    cablewin.h \
    paymentmethodwindow.h \
    messagewindow.h \
    octopuswindow.h \
    wapwindow.h \
    chargefinishwindow.h \
    OCPP/ocppclient.h \
    OCPP/cocppthread.h \
    api_function.h \
    OCPP/ocppconst.h \
    ccharger.h \
    ccomportthread.h \
    T_Type.h \
    database.h \
    crfidthread.h \
    sockettype.h

FORMS    += mainwindow.ui \
    chargingwindow.ui \
    videoplay.ui \
    cablewin.ui \
    paymentmethodwindow.ui \
    messagewindow.ui \
    octopuswindow.ui \
    wapwindow.ui \
    chargefinishwindow.ui \
    sockettype.ui

RESOURCES +=

LIBS    +=-lwiringPi
TRANSLATIONS    +=Zh_cn.ts

#QT  += phonon
QT  += websockets
QT  +=serialport

#QT       -= gui
#CONFIG   += console
