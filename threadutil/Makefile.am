#
# "Makefile.am" for "libupnp/threadutil"
#
# (C) Copyright 2005 Remi Turboult <r3mi@users.sourceforge.net>
#

AM_CPPFLAGS		= -I$(srcdir)/inc -I$(srcdir)/src/inc \
                          -I$(top_srcdir)/upnp/inc

if ENABLE_DEBUG
    AM_CPPFLAGS		+= -DDEBUG -DSTATS
else
    AM_CPPFLAGS		+= -DNDEBUG
endif


lib_LTLIBRARIES		= libthreadutil-1.8.la

libthreadutil_1_8_la_LDFLAGS = -version-info $(LT_VERSION_THREADUTIL)

libthreadutil_1_8_la_SOURCES = \
			inc/FreeList.h \
			src/FreeList.c \
			inc/LinkedList.h \
			src/LinkedList.c \
			inc/ThreadPool.h \
			src/ThreadPool.c \
			inc/TimerThread.h \
			src/TimerThread.c

upnpincludedir		= $(includedir)/upnp-1.8
upnpinclude_HEADERS	= \
			inc/ithread.h

