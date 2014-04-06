#-------------------------------------------------
#
# Project created by QtCreator 2014-04-06T22:38:46
#
#-------------------------------------------------


TEMPLATE = app
QT = core testlib
TARGET = tst_address_hash
CONFIG += console c++11
CONFIG -= app_bundle

load(ultra_benchmark)

LIBS += -L$$BUILDROOT -lultra

SOURCES += tst_address_hash.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
