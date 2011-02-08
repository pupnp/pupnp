#
# "Makefile.am" for "libupnp/threadutil"
#
# (C) Copyright 2005 Remi Turboult <r3mi@users.sourceforge.net>
#

AM_CPPFLAGS		= -I$(srcdir)/inc -I$(srcdir)/src/inc

if ENABLE_DEBUG
    AM_CPPFLAGS		+= -DDEBUG -DSTATS
else
    AM_CPPFLAGS		+= -DNO_DEBUG -DNDEBUG
endif


lib_LTLIBRARIES		= libthreadutil.la

libthreadutil_la_LDFLAGS = -version-info $(LT_VERSION_THREADUTIL)

libthreadutil_la_SOURCES = \
			inc/FreeList.h \
			src/FreeList.c \
			inc/LinkedList.h \
			src/LinkedList.c \
			inc/ThreadPool.h \
			src/ThreadPool.c \
			inc/TimerThread.h \
			src/TimerThread.c

upnpincludedir		= $(includedir)/upnp

upnpinclude_HEADERS	= \
			inc/ithread.h \
			inc/FreeList.h \
			inc/LinkedList.h \
			inc/ThreadPool.h \
			inc/TimerThread.h

