#-------------------------------------------------
#
# Project created by QtCreator 2014-04-23T10:39:13
#
#-------------------------------------------------


TEMPLATE = app
QT =
TARGET = ultra-gtests
CONFIG += console
CONFIG -= app_bundle
QMAKE_CXXFLAGS += -std=gnu++1y
QMAKE_CXXFLAGS_RELEASE += -Ofast

load(ultra_gtest)
load(ultra_jit)

BUILDROOT = ../../bin

CONFIG(debug, debug|release) {
    CONFIG += warn_on
    BUILDSUFFIX = debug
    LIBS += -L$$BUILDROOT -lultra-debug

    TARGET = $$join(TARGET,,, -debug)

} else: CONFIG(release, debug|release) {
    CONFIG += warn_off
    BUILDSUFFIX = release
    LIBS += -L$$BUILDROOT -lultra
}


DESTDIR = $$BUILDROOT
MOC_DIR = $$BUILDROOT/$$BUILDSUFFIX/tests/gtests
OBJECTS_DIR = $$BUILDROOT/$$BUILDSUFFIX/tests/gtests


HEADERS += \
    benchmark.h \
    mock_types.h

SOURCES += \
    benchmark.cpp \
    main.cpp \
    test_address.cpp \
    test_concurrent_queue.cpp \
    test_ordered_lock.cpp \
    test_libjit.cpp
