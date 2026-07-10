QT += core gui widgets serialport printsupport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    pages/connectionpage.cpp \
    widgets/consolewidget.cpp \
    core/databasemanager.cpp \
    pages/datasetpage.cpp \
    widgets/leftmenuwidget.cpp \
    pages/livestreampage.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    qcustomplot/qcustomplot.cpp \
    core/serialport.cpp \
    pages/sessionpage.cpp \
    pages/settingspage.cpp \
    widgets/statusbarwidget.cpp

HEADERS += \
    pages/connectionpage.h \
    widgets/consolewidget.h \
    core/databasemanager.h \
    pages/datasetpage.h \
    widgets/leftmenuwidget.h \
    pages/livestreampage.h \
    ui/mainwindow.h \
    qcustomplot/qcustomplot.h \
    core/serialport.h \
    pages/sessionpage.h \
    pages/settingspage.h \
    models/motionsample.h \
    widgets/statusbarwidget.h

FORMS += \
    ui/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
