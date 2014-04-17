#-------------------------------------------------
#
# Project created by QtCreator 2014-04-09T01:37:11
#
#-------------------------------------------------


TEMPLATE = app
QT = core testlib
TARGET = tst_maps
CONFIG += console c++11
CONFIG -= app_bundle

load(ultra_benchmark)
load(ultra_cds)

LIBS += -L$$BUILDROOT -lultra

SOURCES += tst_maps.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
