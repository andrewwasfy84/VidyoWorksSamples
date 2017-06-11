#-------------------------------------------------
#
# Project created by QtCreator 2016-06-08T13:26:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtPrecall
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    vsroomlink.cpp \
    VsPlugin.cpp \
    VsStatus.cpp

HEADERS  += mainwindow.h \
    vsroomlink.h \
    VsPlugin.h \
    VsStatus.h \
    VDSound.h

FORMS    += mainwindow.ui \
    vsroomlink.ui

INCLUDEPATH +=\
                VidyoClient/include

macx {
        QMAKE_INFO_PLIST = resources/macx/info.plist
        APP_WAV_FILES.files = ../resources/sound/testtone.wav
        SOURCES += mac/VDSound.cpp

}

win32 {
       RC_FILE = resources/win32/win32.rc
       SOURCES += win32/VDSound.cpp
}


unix {
        macx {
            LIBS += \
            ../QtPrecall/VidyoClient/lib/mac/VidyoClientDll.dylib

        } else {
        LIBS += \
        VidyoClient/lib/linux/libVidyoClientDll.so\
        -LVidyoClient/lib\
        -lblkid\
        -lXrandr\
        -lanl\
        -lpthread\
        -lGL -lGLU -lX11\
        -lrt
        }
}
win32 {
        LIBS += \
        -L..\\QtPrecall\\VidyoClient\\lib\\windows\
        VidyoClientDll.lib
}

