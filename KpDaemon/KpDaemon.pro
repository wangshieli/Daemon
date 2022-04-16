QT -= gui

CONFIG += c++11 #console
CONFIG -= app_bundle

LIBS += -lDbgHelp

CONFIG(debug, debug | release){
    DESTDIR = $$PWD/../bin/debug
} else {
    DESTDIR = $$PWD/../bin/release
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        DaemonTool.cpp \
        PeAnalyzer.cpp \
        WorkThread.cpp \
        main.cpp

HEADERS += \
    DaemonTool.h \
    PeAnalyzer.h \
    WorkThread.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
