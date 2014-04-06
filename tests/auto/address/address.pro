#-------------------------------------------------
#
# Project created by QtCreator 2014-04-06T18:30:29
#
#-------------------------------------------------


TEMPLATE = app
QT = core testlib
TARGET = tst_address
CONFIG += console c++11
CONFIG -= app_bundle

load(ultra_autotest)

SOURCES += tst_address.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
