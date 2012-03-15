#
# "Makefile.am" for "libupnp/upnp"
#
# Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

SUBDIRS = doc . sample

AM_CPPFLAGS = \
	-I$(srcdir)/inc \
	-I$(top_srcdir)/threadutil/inc \
	-I$(top_srcdir)/ixml/inc

LDADD = \
	libupnp.la \
	$(top_builddir)/threadutil/libthreadutil.la \
	$(top_builddir)/ixml/libixml.la


upnpincludedir = $(includedir)/upnp
upnpinclude_HEADERS = \
	inc/UpnpString.h \
	inc/upnp.h \
	inc/upnpdebug.h \
	inc/UpnpGlobal.h \
	inc/UpnpInet.h \
	inc/UpnpIntTypes.h \
	inc/UpnpStdInt.h \
	inc/UpnpUniStd.h

nodist_upnpinclude_HEADERS = inc/upnpconfig.h 
if ENABLE_TOOLS
upnpinclude_HEADERS += inc/upnptools.h 
endif


lib_LTLIBRARIES = libupnp.la

libupnp_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/src/inc 

libupnp_la_LDFLAGS = \
	-version-info $(LT_VERSION_UPNP) \
	-export-symbols-regex '^Upnp.*' \
	$(top_builddir)/threadutil/libthreadutil.la \
	$(top_builddir)/ixml/libixml.la

libupnp_la_SOURCES = \
	src/inc/config.h \
	src/inc/client_table.h \
	src/inc/gena.h \
	src/inc/gena_ctrlpt.h \
	src/inc/gena_device.h \
	src/inc/global.h \
	src/inc/gmtdate.h \
	src/inc/httpparser.h \
	src/inc/httpreadwrite.h \
	src/inc/md5.h \
	src/inc/membuffer.h \
	src/inc/miniserver.h \
	src/inc/netall.h \
	src/inc/parsetools.h \
	src/inc/server.h \
	src/inc/service_table.h \
	src/inc/soaplib.h \
	src/inc/sock.h \
	src/inc/statcodes.h \
	src/inc/statuscodes.h \
	src/inc/strintmap.h \
	src/inc/ssdplib.h \
	src/inc/sysdep.h \
	src/inc/unixutil.h \
	src/inc/upnpapi.h \
	src/inc/upnp_timeout.h \
	src/inc/uri.h \
	src/inc/urlconfig.h \
	src/inc/upnputil.h \
	src/inc/uuid.h \
	src/inc/VirtualDir.h \
	src/inc/webserver.h

# ssdp
if ENABLE_SSDP
libupnp_la_SOURCES += \
        src/ssdp/ssdp_ResultData.h \
	src/ssdp/ssdp_device.c \
	src/ssdp/ssdp_ctrlpt.c \
	src/ssdp/ssdp_server.c
endif

# soap
if ENABLE_SOAP
libupnp_la_SOURCES += \
	src/soap/soap_device.c \
	src/soap/soap_ctrlpt.c \
	src/soap/soap_common.c
endif

# genlib
libupnp_la_SOURCES += \
	src/genlib/miniserver/miniserver.c \
	src/genlib/service_table/service_table.c \
	src/genlib/util/membuffer.c \
	src/genlib/util/strintmap.c \
	src/genlib/util/upnp_timeout.c \
	src/genlib/util/util.c \
	src/genlib/client_table/client_table.c \
	src/genlib/net/sock.c \
	src/genlib/net/http/httpparser.c \
	src/genlib/net/http/httpreadwrite.c \
	src/genlib/net/http/statcodes.c \
	src/genlib/net/http/webserver.c \
	src/genlib/net/http/parsetools.c \
	src/genlib/net/uri/uri.c

# gena
if ENABLE_GENA
libupnp_la_SOURCES += \
	src/gena/gena_device.c \
	src/gena/gena_ctrlpt.c \
	src/gena/gena_callback2.c
endif

# api
libupnp_la_SOURCES += \
	src/api/UpnpString.c \
	src/api/upnpapi.c

if ENABLE_TOOLS
libupnp_la_SOURCES += src/api/upnptools.c
endif

if ENABLE_DEBUG
libupnp_la_SOURCES += src/api/upnpdebug.c 
endif


# uuid
if ENABLE_UUID
libupnp_la_SOURCES += \
	src/uuid/md5.c \
	src/uuid/sysdep.c \
	src/uuid/uuid.c
endif


# urlconfig
libupnp_la_SOURCES += src/urlconfig/urlconfig.c


# inet_pton (needed on Win32, compiles to nothing elsewhere)
libupnp_la_SOURCES += \
	src/inet_pton.c \
	src/inc/inet_pton.h


# check / distcheck tests
check_PROGRAMS = test_init
TESTS = test_init
test_init_SOURCES = test/test_init.c


EXTRA_DIST = \
	LICENSE \
	m4/libupnp.m4 \
	src/win_dll.c


CLEANFILES = \
	IUpnpErrFile.txt \
	IUpnpInfoFile.txt

