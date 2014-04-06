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


### Building settings ###

DESTDIR = ../bin
BOOST_PATH = $$PWD/../../boost_1_55_0
CDS_PATH = $$PWD/../../cds-1.5.0
CDS_LIB_PATH = $$CDS_PATH/bin/gcc-amd64-linux-0
INCLUDEPATH += $$BOOST_PATH $$CDS_PATH
DEPENDPATH += $$BOOST_PATH $$CDS_PATH

CONFIG(debug, debug|release) {
    CONFIG += warn_on
    MOC_DIR = ../bin/debug
    OBJECTS_DIR = ../bin/debug

    LIBS += -L$$CDS_LIB_PATH -lcds-debug

    unix: LIBS += -L$$BOOST_PATH/stage/lib/ \
                -lboost_system-mt-s -lboost_context-mt-s
    else: win32: LIBS += -L$$BOOST_PATH/stage/lib/debug/ \
                -lboost_system-mt-d  -lboost_context-mt-d

    *g++*: QMAKE_POST_LINK += \
        $$QMAKE_OBJCOPY --only-keep-debug \
            $$DESTDIR/$(TARGET) $$DESTDIR/$(TARGET).debug $$escape_expand(\\n\\t) \
        $$QMAKE_OBJCOPY --strip-debug \
            $$DESTDIR/$(TARGET) $$escape_expand(\\n\\t) \
        $$QMAKE_OBJCOPY --add-gnu-debuglink=$$DESTDIR/$(TARGET).debug \
            $$DESTDIR/$(TARGET) $$escape_expand(\\n\\t)

    TARGET = $$join(TARGET,,, -debug)

} else: CONFIG(release, debug|release) {
    CONFIG += warn_off
    MOC_DIR = ../bin/release
    OBJECTS_DIR = ../bin/release

    LIBS += -L$$CDS_LIB_PATH -lcds

    unix: LIBS += -L$$BOOST_PATH/stage/lib/ \
                -lboost_system-mt-s -lboost_context-mt-s
    else: win32: LIBS += -L$$BOOST_PATH/stage/lib/release/ \
                -lboost_system-mt-s -lboost_context-mt-s

    *g++*: QMAKE_POST_LINK += \
        $$QMAKE_OBJCOPY --strip-unneeded $$DESTDIR/$(TARGET) $$escape_expand(\\n\\t)
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
