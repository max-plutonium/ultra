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

CONFIG(debug, debug|release) {
    CONFIG += warn_on
    MOC_DIR = ../bin/debug
    OBJECTS_DIR = ../bin/debug

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

    *g++*: QMAKE_POST_LINK += \
        $$QMAKE_OBJCOPY --strip-unneeded $$DESTDIR/$(TARGET) $$escape_expand(\\n\\t)
}


*g++* {
    QMAKE_CXXFLAGS += \
        -std=gnu++11 -pthread -funwind-tables \
        -Wno-write-strings -Wno-unused-local-typedefs
    QMAKE_CXXFLAGS_RELEASE += -fvisibility=hidden -fvisibility-inlines-hidden
    win32*: LIBS += -lpthread
}


HEADERS += ultra.h \
    ultra_global.h

PRIVATE_HEADERS +=

SOURCES += ultra.cpp


headerTarget.path = ../include
headerTarget.files = $$HEADERS

target.path = ../lib
INSTALLS += target headerTarget
