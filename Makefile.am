#
# Top-level "Makefile.am" for libupnp
#
# Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-debug --enable-samples

SUBDIRS = \
	ixml \
	upnp \
	docs

EXTRA_DIST = \
	docs/Doxyfile \
	libupnp.pc.in \
	libupnp.spec \
	build/msvc/inttypes.h \
	build/msvc/stdint.h


# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir = $(libdir)/pkgconfig
pkgconfigexec_DATA = libupnp.pc

$(pkgconfigexec_DATA): config.status

CLEANFILES = build/inc/autoconfig.h build/inc/upnpconfig.h
