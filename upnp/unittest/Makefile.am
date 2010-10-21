#
# "Makefile.am" for "libupnp/upnp/unittest"
#
# Copyright (C) 2010 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
#

AM_CPPFLAGS = \
	-I$(top_srcdir)/upnp/inc \
	-I$(top_srcdir)/threadutil/inc \
	-I$(top_srcdir)/ixml/inc \
	-I$(top_srcdir)/upnp/src/inc

LDADD = \
	$(top_builddir)/upnp/libupnp.la \
	$(top_builddir)/threadutil/libthreadutil.la \
	$(top_builddir)/ixml/libixml.la

#unittest
noinst_PROGRAMS = unittest

unittest_CPPFLAGS = \
	$(AM_CPPFLAGS)

unittest_SOURCES = \
	main.c \
	templates/FirstObject.c \
	templates/FirstObject.h \
	templates/templates.c \
	templates/templates.h \
	templates/SecondObject.c \
	templates/SecondObject.h \
	templates/TheStruct.h

