QT += core gui widgets serialport printsupport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    pages/connectionpage.cpp \
    pages/createdbdialog.cpp \
    pages/databasepage.cpp \
    pages/opendbdialog.cpp \
    widgets/consolewidget.cpp \
    core/databasemanager.cpp \
    widgets/leftmenuwidget.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    qcustomplot/qcustomplot.cpp \
    core/serialport.cpp \
    widgets/statusbarwidget.cpp

HEADERS += \
    models/MotionType.h \
    models/packettype.h \
    models/statusport.h \
    pages/connectionpage.h \
    pages/createdbdialog.h \
    pages/databasepage.h \
    pages/opendbdialog.h \
    widgets/consolewidget.h \
    core/databasemanager.h \
    widgets/leftmenuwidget.h \
    ui/mainwindow.h \
    qcustomplot/qcustomplot.h \
    core/serialport.h \
    models/loglevel.h \
    models/motionsample.h \
    models/portconfig.h \
    widgets/statusbarwidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
