#-------------------------------------------------
#
# Project created by QtCreator 2014-04-06T13:56:37
#
#-------------------------------------------------


TEMPLATE = lib
QT =
TARGET = ultra
DEFINES += ULTRA_SHARED
VERSION = 0.1.1
QMAKE_CXXFLAGS += -std=gnu++14 -fopenmp
QMAKE_CXXFLAGS_RELEASE += -Ofast

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

*g++*|*clang {
    QMAKE_CXXFLAGS += \
        -std=c++1y -pthread -funwind-tables \
        -Wno-write-strings -Wno-unused-local-typedefs \
        -Wunreachable-code -Woverloaded-virtual
    QMAKE_CXXFLAGS_RELEASE += -fno-omit-frame-pointer #-fvisibility=hidden -fvisibility-inlines-hidden
    win32 {
        LIBS += -lpthread
        DEFINES += _GLIBCXX_HAS_GTHREADS
    }
}

LIBS += -lgomp -lprotobuf

MSGFILES = \
    msg.proto

protoc.name = protoc
protoc.input = MSGFILES
protoc.output = ${QMAKE_FILE_BASE}.pb.cc
protoc.commands = protoc --cpp_out=. ${QMAKE_FILE_IN}
protoc.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += protoc


### Files ###

HEADERS += \
    address.h \
    core/concurrent_queue.h \
    core/locks.h \
    core/result.h \
    core/system.h \
    core/thread_pool.h \
    ultra.h \
    ultra_global.h \
    vm.h \
    core/action.h \
    logic_time.h \
    core/ioservice_pool.h \
    port.h \
    core/core_p.h \
    core/schedulers.h \
    core/network_session.h \
    messages.h \
    genetic.h \
    node.h \
    rbm.h \
    back_prop.h \
    util.h \
    stacked_rbm.h \
    denoising_autoencoder.h

PRIVATE_HEADERS = $$files(*_p.h)
PUBLIC_HEADERS = $$HEADERS
PUBLIC_HEADERS -= $$PRIVATE_HEADERS

SOURCES += \
    address.cpp \
    core/concurrent_queue.ipp \
    core/result.cpp \
    core/system.cpp \
    core/thread_pool.cpp \
    ultra.cpp \
    vm.cpp \
    logic_time.cpp \
    core/ioservice_pool.cpp \
    port.cpp \
    core/core.cpp \
    core/schedulers.cpp \
    core/network_session.cpp \
    messages.cpp \
    genetic.cpp \
    node.cpp \
    core/locks.cpp \
    rbm.cpp \
    back_prop.cpp \
    util.cpp \
    stacked_rbm.cpp \
    denoising_autoencoder.cpp

### Install settings ###

headerTarget.path = ../include
headerTarget.files = $$PUBLIC_HEADERS

target.path = ../lib
INSTALLS += target headerTarget

DISTFILES += \
    ../examples/core/action.cpp \
    ../examples/core/concurrent_queue.cpp \
    ../examples/core/thread_pool.cpp \
    ../examples/ultra.cpp
