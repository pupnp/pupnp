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
noinst_PROGRAMS += upnp_tv_ctrlpt
upnp_tv_ctrlpt_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvctrlpt
if ENABLE_DEVICE
noinst_PROGRAMS += upnp_tv_combo
upnp_tv_combo_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvcombo
endif
endif
if ENABLE_DEVICE
noinst_PROGRAMS += upnp_tv_device
upnp_tv_device_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(srcdir)/common/ \
	-I$(srcdir)/tvdevice
endif
endif


upnp_tv_device_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	tvdevice/upnp_tv_device.c \
	tvdevice/upnp_tv_device.h \
	tvdevice/linux/upnp_tv_device_main.c


upnp_tv_ctrlpt_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	tvctrlpt/upnp_tv_ctrlpt.c \
	tvctrlpt/upnp_tv_ctrlpt.h \
	tvctrlpt/linux/upnp_tv_ctrlpt_main.c

upnp_tv_combo_SOURCES = \
	common/sample_util.c \
	common/sample_util.h \
	tvcombo/upnp_tv_ctrlpt.c \
	tvcombo/upnp_tv_ctrlpt.h \
	tvcombo/upnp_tv_device.c \
	tvcombo/upnp_tv_device.h \
	tvcombo/linux/upnp_tv_combo_main.c


if WITH_DOCUMENTATION
examplesdir = $(docdir)/examples
examples_DATA = \
		$(upnp_tv_ctrlpt_SOURCES) \
		$(upnp_tv_device_SOURCES)
endif


EXTRA_DIST = \
	web/tvcombodesc.xml \
	web/tvcontrolSCPD.xml \
	web/tvdevicedesc.xml \
	web/tvdevicepres.html \
	web/tvpictureSCPD.xml

