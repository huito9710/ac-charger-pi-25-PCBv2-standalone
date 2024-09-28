#-------------------------------------------------
#
# Project created by QtCreator 2016-05-11T14:59:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
#QT  += phonon
QT  += websockets
QT  +=serialport
QT  += sql

#QT       -= gui
TARGET = ChargerV2_STA
TEMPLATE = app


SOURCES += main.cpp\
    OCPP/callerrormessage.cpp \
    OCPP/callmessage.cpp \
    OCPP/callresultmessage.cpp \
    OCPP/ocppTypes.cpp \
    OCPP/ocppoutmessagequeue.cpp \
    OCPP/ocpptransactionmessagerequester.cpp \
    OCPP/rpcmessage.cpp \
    authorizationcache.cpp \
    chargerconfig.cpp \
    chargerequestpassword.cpp \
    chargerrequestrfid.cpp \
    chargingmodeimmediatewin.cpp \
    connstatusindicator.cpp \
    evchargingtransaction.cpp \
    evchargingtransactionregistry.cpp \
    firmwaredownloadstate.cpp \
    firmwareupdatescheduler.cpp \
    localauthlist.cpp \
    localauthlistdb.cpp \
        mainwindow.cpp \
    chargingwindow.cpp \
    passwordmgr.cpp \
    passwordupdater.cpp \
    powermeterrecfile.cpp \
    qrcodegen/QrCode.cpp \
    rfidreaderwindow.cpp \
    softwaredownloader.cpp \
    statusledctrl.cpp \
    tests/authorizationcachetests.cpp \
    tests/sanitytest.cpp \
    transactionrecordfile.cpp \
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
    ccomportthreadv2.cpp \
    crfidthread.cpp \
    sockettype.cpp \
    passwordwin.cpp \
    setchargingtimewin.cpp \
    setdelaychargingwin.cpp \
    chargingmodewin.cpp \
    delaychargingwin.cpp \
    waittingwindow.cpp \
    rfidwindow.cpp \
    reservewindow.cpp \
    popupmsgwin.cpp \
   libnfc/buses/i2c.c \
   libnfc/buses/spi.c \
   libnfc/buses/uart.c \
   libnfc/buses/usbbus.c \
   libnfc/chips/pn53x.c \
   libnfc/drivers/acr122_usb.c \
   libnfc/drivers/acr122s.c \
   libnfc/drivers/pn532_i2c.c \
   libnfc/drivers/pn532_spi.c \
   libnfc/drivers/pn532_uart.c \
   libnfc/drivers/pn53x_usb.c \
   libnfc/utils/mifare.c \
   libnfc/conf.c \
   libnfc/iso14443-subr.c \
   libnfc/log-internal.c \
   libnfc/log.c \
   libnfc/mirror-subr.c \
   libnfc/nfc-device.c \
   libnfc/nfc-emulation.c \
   libnfc/nfc-internal.c \
   libnfc/nfc.c \
   libnfc/target-subr.c \
    distancesensorthread.cpp \
    suspendchargingwin.cpp

win32 {
    SOURCES += hostplatform_windows.cpp
}
unix {
    SOURCES += hostplatform_linux.cpp
}

CONFIG(debug) {
    SOURCES += tests/localauthlisttests.cpp
}

HEADERS  += mainwindow.h \
    OCPP/callerrormessage.h \
    OCPP/callmessage.h \
    OCPP/callresultmessage.h \
    OCPP/ocppTypes.h \
    OCPP/ocppoutmessagequeue.h \
    OCPP/ocpptransactionmessagerequester.h \
    OCPP/rpcmessage.h \
    authorizationcache.h \
    chargerconfig.h \
    chargerequestpassword.h \
    chargerrequestrfid.h \
    chargingmodeimmediatewin.h \
    chargingwindow.h \
    connstatusindicator.h \
    const.h \
    evchargingtransaction.h \
    evchargingtransactionregistry.h \
    firmwaredownloadstate.h \
    firmwareupdatescheduler.h \
    hostplatform.h \
    localauthlist.h \
    localauthlistdb.h \
    logconfig.h \
    passwordmgr.h \
    passwordupdater.h \
    powermeterrecfile.h \
    qrcodegen/QrCode.hpp \
    rfidreaderwindow.h \
    softwaredownloader.h \
    statusledctrl.h \
    tests/authorizationcachetests.h \
    tests/sanitytest.h \
    transactionrecordfile.h \
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
    ccomportthreadv2.h \
    T_Type.h \
    crfidthread.h \
    sockettype.h \
    passwordwin.h \
    setchargingtimewin.h \
    setdelaychargingwin.h \
    chargingmodewin.h \
    delaychargingwin.h \
    gvar.h \
    waittingwindow.h \
    rfidwindow.h \
    reservewindow.h \
    popupmsgwin.h \
   libnfc/buses/i2c.h \
   libnfc/buses/spi.h \
   libnfc/buses/uart.h \
   libnfc/buses/usbbus.h \
   libnfc/chips/pn53x-internal.h \
   libnfc/chips/pn53x.h \
   libnfc/drivers/acr122_usb.h \
   libnfc/drivers/acr122s.h \
   libnfc/drivers/pn532_i2c.h \
   libnfc/drivers/pn532_spi.h \
   libnfc/drivers/pn532_uart.h \
   libnfc/drivers/pn53x_usb.h \
   libnfc/drivers/pn71xx.h \
   libnfc/utils/mifare.h \
   libnfc/conf.h \
   libnfc/drivers.h \
   libnfc/iso7816.h \
   libnfc/log-internal.h \
   libnfc/log.h \
   libnfc/mirror-subr.h \
   libnfc/nfc-internal.h \
   libnfc/target-subr.h \
   distancesensorthread.h \
   suspendchargingwin.h

CONFIG(debug) {
    HEADERS += tests/localauthlisttests.h
}

FORMS    += mainwindow.ui \
    chargingwindow.ui \
    videoplay.ui \
    cablewin.ui \
    paymentmethodwindow.ui \
    messagewindow.ui \
    octopuswindow.ui \
    wapwindow.ui \
    chargefinishwindow.ui \
    sockettype.ui \
    passwordwin.ui \
    setchargingtimewin.ui \
    setdelaychargingwin.ui \
    chargingmodewin.ui \
    delaychargingwin.ui \
    waittingwindow.ui \
    rfidwindow.ui \
    reservewindow.ui \
    popupmsgwin.ui \
    suspendchargingwin.ui

RESOURCES += \
    Jp.qrc

unix {
    LIBS    +=-lwiringPi
    LIBS    +=-lvlc
    LIBS    +=-lusb
#    LIBS    +=-lmodbus
}

TRANSLATIONS    += ChargerV2_zh_HK.ts


#CONFIG   += console

win32 {
    DEFINES += WINDOWS_NATIVE_BUILD
}

unix {
    DEFINES += VLC_AVAILABLE
}

DEFINES += DRIVER_PN532_I2C_ENABLED
DEFINES += DRIVER_PN532_UART_ENABLED

# Disable qDebug() output from Release builds, use log-category to turn off
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
