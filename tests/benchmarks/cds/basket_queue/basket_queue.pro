#-------------------------------------------------
#
# Project created by QtCreator 2014-04-07T01:18:50
#
#-------------------------------------------------


TEMPLATE = app
QT = core testlib
TARGET = tst_basket_queue
CONFIG += console c++11
CONFIG -= app_bundle

load(ultra_benchmark)
load(ultra_cds)

SOURCES += tst_basket_queue.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
