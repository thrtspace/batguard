#-------------------------------------------------
#
# Project created by QtCreator 2018-01-17T11:10:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = batguard
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    camera_pg.cpp \
    edge_checker.cpp

HEADERS += \
        mainwindow.h \
    edge_checker.h

FORMS += \
        mainwindow.ui

INCLUDEPATH += /usr/include/spinnaker/spinc /usr/local/include

LIBS += -L/usr/lib -L/usr/lib/x86_64-linux-gnu/mesa -lpthread

LIBS += -lSpinnaker -lSpinnaker_C -lopencv_core -lopencv_imgproc -lopencv_imgcodecs

RESOURCES += \
    batguard.qrc
