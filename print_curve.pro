#-------------------------------------------------
#
# Project created by QtCreator 2013-07-20T11:54:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = print_curve
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

INCLUDEPATH     += C:\sunxin\libs\sp_xmlread
LIBS            += C:\sunxin\libs\sp_xmlread\release\libsp_xmlread.a

RESOURCES += \
    logo.qrc
QT += printsupport
