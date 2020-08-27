QT -= gui

CONFIG += c++11 console -fexceptions
CONFIG -= app_bundle
### sudo apt-get install libpng-dev
### sudo apt-get install libv4l-dev
### sudo apt-get install libjpeg-dev

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += cimg_display=0
DEFINES += cimg_use_jpeg

INCLUDEPATH += ../common
# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../rapidjson/include

SOURCES += \
        ../common/vigenere.cpp \
        config.cpp \
        devvideo.cpp \
        frame.cpp \
        frameclient.cpp \
        jpeger.cpp \
        main.cpp \
        md5.cpp \
        mmotion.cpp \
        osthread.cpp \
        sock.cpp \
        streamq.cpp \
        urlinfo.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../common/vigenere.h \
    ../rapidjson/include/rapidjson/rapidjson.h \
    cimg.h \
    devvideo.h \
    ffmt.h \
    frame.h \
    frameclient.h \
    jpeger.h \
    main.h \
    md5.h \
    mmotion.h \
    osthread.h \
    singleton.h \
    sock.h \
    streamq.h \
    urlinfo.h

unix|win32: LIBS += -lpthread -ljpeg -lv4l2

DISTFILES += \
    ../../../../../../var/www/html/accesspi.php \
    ../../../../../../var/www/html/pisica.php

