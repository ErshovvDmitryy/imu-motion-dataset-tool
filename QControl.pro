QT += core gui widgets serialport printsupport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    core/databasemanager.cpp \
    core/motionrecorder.cpp \
    core/protocol/frameparser.cpp \
    core/protocol/messagedecoder.cpp \
    core/serialport.cpp \
    main.cpp \
    models/builtinschemas.cpp \
    models/datapacket.cpp \
    models/dataschema.cpp \
    models/datavalue.cpp \
    models/imuadapter.cpp \
    models/schemastore.cpp \
    modules/plot/plotconfig.cpp \
    modules/slicer/windowslicer.cpp \
    modules/widgets/plotconfigdialog.cpp \
    modules/widgets/plotmanager.cpp \
    pages/connectionpage.cpp \
    pages/createdbdialog.cpp \
    pages/databasepage.cpp \
    pages/exportcsv.cpp \
    pages/messageeditpage.cpp \
    pages/opendbdialog.cpp \
    qcustomplot/qcustomplot.cpp \
    ui/mainwindow.cpp \
    widgets/consolewidget.cpp \
    widgets/leftmenuwidget.cpp \
    widgets/statusbarwidget.cpp

HEADERS += \
    core/databasemanager.h \
    core/motionrecorder.h \
    core/protocol/frameparser.h \
    core/protocol/messagedecoder.h \
    core/serialport.h \
    models/AnalysisConfig.h \
    models/MotionType.h \
    models/builtinschemas.h \
    models/datapacket.h \
    models/dataschema.h \
    models/datavalue.h \
    models/imuadapter.h \
    models/loglevel.h \
    models/motionsample.h \
    models/packettype.h \
    models/portconfig.h \
    models/schemastore.h \
    models/statusport.h \
    modules/plot/plotconfig.h \
    modules/slicer/windowslicer.h \
    modules/widgets/plotconfigdialog.h \
    modules/widgets/plotmanager.h \
    pages/connectionpage.h \
    pages/createdbdialog.h \
    pages/databasepage.h \
    pages/exportcsv.h \
    pages/messageeditpage.h \
    pages/opendbdialog.h \
    qcustomplot/qcustomplot.h \
    ui/mainwindow.h \
    widgets/consolewidget.h \
    widgets/leftmenuwidget.h \
    widgets/statusbarwidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
