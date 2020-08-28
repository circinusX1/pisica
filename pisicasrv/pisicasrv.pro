QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += DEBUG
INCLUDEPATH += ../common
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O0  -Wno-unused-parameter

QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG += -O0  -Wno-unused-parameter

SOURCES += main.cpp \
    ../common/vigenere.cpp \
    md5.cpp \
    skbase.cpp \
    skcam.cpp \
    skcamsq.cpp \
    skimg.cpp \
    sks.cpp \
    sksrv.cpp \
    skweb.cpp \
    sock.cpp \
    vf.cpp \
    logfile.cpp

HEADERS += \
    ../common/config.h \
    ../common/vigenere.h \
    osthread.h \
    pks.h \
    request.h \
    skbase.h \
    skcam.h \
    skcamsq.h \
    skimg.h \
    sks.h \
    sksrv.h \
    skweb.h \
    sock.h \
    main.h \
    md5.h \
    encrypt.h \
    logfile.h \
    vf.h

DISTFILES += \
    ../pisicaweb/local.php \
    ../pisicaweb/pisica.php

