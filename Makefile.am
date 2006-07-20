# $Id: Makefile.am,v 1.5 2006/04/29 20:12:50 r3mi Exp $
#
# Top-level "Makefile.am" for libupnp
#
# Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

ACLOCAL_AMFLAGS 	= -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-debug --enable-samples

SUBDIRS 		= ixml threadutil upnp docs/dist


EXTRA_DIST		= libupnp.pc.in LICENSE THANKS libupnp.spec \
			build/libupnp.bpf \
			build/libupnp.bpr \
			build/libupnp.dsp \
			build/libupnp.dsw \
			build/inc/autoconfig.h \
			build/inc/config.h \
			build/inc/upnpconfig.h


# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir 	= $(libdir)/pkgconfig
pkgconfigexec_DATA 	= libupnp.pc

$(pkgconfigexec_DATA): config.status


if WITH_DOCDIR
    docdir		= @DOCDIR@
    doc_DATA 		= LICENSE README NEWS TODO THANKS
endif


CLEANFILES		= IUpnpErrFile.txt IUpnpInfoFile.txt





