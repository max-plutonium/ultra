#-------------------------------------------------
#
# Project created by QtCreator 2014-04-06T13:56:37
#
#-------------------------------------------------


TEMPLATE = lib
QT -= core gui
TARGET = ultra
DEFINES += ULTRA_SHARED
VERSION = 0.0.0

load(ultra_cds)
load(ultra_boost)


### Building settings ###

DESTDIR = ../bin

CONFIG(debug, debug|release) {
    CONFIG += warn_on
    MOC_DIR = ../bin/debug
    OBJECTS_DIR = ../bin/debug

    CONFIG += separate_debug_info

    TARGET = $$join(TARGET,,, -debug)

} else: CONFIG(release, debug|release) {
    CONFIG += warn_off
    MOC_DIR = ../bin/release
    OBJECTS_DIR = ../bin/release

    *g++*: QMAKE_POST_LINK += \
        $$QMAKE_OBJCOPY --strip-unneeded \
            $$DESTDIR/$(TARGET) $$escape_expand(\\n\\t)
}


### Compiler settings ###

*g++* {
    QMAKE_CXXFLAGS += \
        -std=gnu++11 -pthread -funwind-tables \
        -Wno-write-strings -Wno-unused-local-typedefs
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden -fvisibility-inlines-hidden
    win32*: LIBS += -lpthread
}


### Files ###

HEADERS += \
    ultra.h \
    ultra_global.h \
    core.h \
    address.h

PRIVATE_HEADERS = $$files(*_p.h)
PUBLIC_HEADERS = $$HEADERS
PUBLIC_HEADERS -= $$PRIVATE_HEADERS

SOURCES += \
    ultra.cpp \
    address.cpp


### Install settings ###

headerTarget.path = ../include
headerTarget.files = $$PUBLIC_HEADERS

target.path = ../lib
INSTALLS += target headerTarget
