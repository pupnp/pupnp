#
# "Makefile.am" for "libupnp/upnp/sample"
#
# Copyright (C) 2007 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
#

AM_CPPFLAGS = \
	-I$(top_srcdir)/upnp/inc \
	-I$(top_srcdir)/threadutil/inc \
	-I$(top_srcdir)/ixml/inc

LDADD = \
	$(top_builddir)/upnp/libupnp.la \
	$(top_builddir)/threadutil/libthreadutil.la \
	$(top_builddir)/ixml/libixml.la

# samples
noinst_PROGRAMS =
if ENABLE_SAMPLES
if ENABLE_CLIENT
noinst_PROGRAMS += tv_ctrlpt
tv_ctrlpt_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvctrlpt
if ENABLE_DEVICE
noinst_PROGRAMS += tv_combo
tv_combo_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvcombo
endif
endif
if ENABLE_DEVICE
noinst_PROGRAMS += tv_device
tv_device_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvdevice
endif
endif

tv_device_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	common/tv_device.c \
	common/tv_device.h \
	linux/tv_device_main.c

tv_ctrlpt_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	common/tv_ctrlpt.c \
	common/tv_ctrlpt.h \
	linux/tv_ctrlpt_main.c

tv_combo_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	common/tv_ctrlpt.c \
	common/tv_ctrlpt.h \
	common/tv_device.c \
	common/tv_device.h \
	linux/tv_combo_main.c

if WITH_DOCUMENTATION
examplesdir = $(docdir)/examples
examples_DATA = \
	$(tv_ctrlpt_SOURCES) \
	$(tv_device_SOURCES)
endif

EXTRA_DIST = \
	web/tvcombodesc.xml \
	web/tvcontrolSCPD.xml \
	web/tvdevicedesc.xml \
	web/tvdevicepres.html \
	web/tvpictureSCPD.xml

