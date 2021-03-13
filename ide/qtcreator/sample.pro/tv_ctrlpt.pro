TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TOPDIR = ../..

INCLUDEPATH += \
        $$TOPDIR/upnp/sample/common \
        $$TOPDIR/upnp/inc \
        $$TOPDIR/ixml/inc \

HEADERS += \
        $$TOPDIR/upnp/sample/common/sample_util.h \
        $$TOPDIR/upnp/sample/common/tv_ctrlpt.h \

SOURCES += \
        $$TOPDIR/upnp/sample/linux/tv_ctrlpt_main.c \
        $$TOPDIR/upnp/sample/common/sample_util.c \
        $$TOPDIR/upnp/sample/common/tv_ctrlpt.c \

LIBS += \
        -lpthread \
        -L$$TOPDIR/upnp/.libs -lupnp \
        -L$$TOPDIR/ixml/.libs -lixml \

