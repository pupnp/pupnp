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
	CMakeLists.txt \
	cmake/CmDaB.cmake \
	cmake/autoheader.cmake \
	cmake/options.cmake \
	cmake/post-test.cmake \
	cmake/test-functions.cmake \
	docs/Doxyfile \
	fuzzer/FuzzIxml.c \
	fuzzer/build.sh \
	fuzzer/CMakeLists.txt \
	fuzzer/readme.md \
	gtest/README.md \
	gtest/CMakeLists.txt \
	gtest/tools \
	gtest/tools/compile.sh \
	gtest/tools/README \
	gtest/tools/test_tools.cpp \
	gtest/tools/tools.cpp \
	gtest/test_UpnpHttpHeaderList.cpp \
	gtest/test_template.cpp \
	gtest/test_upnpapi.cpp \
	gtest/TODO \
	scripts/comp-autotools.sh \
	scripts/comp-cmake.sh \
	scripts/find_out_bz2_name.sh \
	scripts/home_keeping.sh \
	scripts/parse_release.sh \
	scripts/restore_pupnp_files.sh \
	IXML.cmake.in \
	UPNP.cmake.in \
	ixml/CMakeLists.txt \
	ixml/test/CMakeLists.txt \
	upnp/CMakeLists.txt \
	upnp/test/CMakeLists.txt \
	upnp/sample/CMakeLists.txt \
	upnp/inc/upnpconfig.h.cm \
	upnp/sample/common/config_sample.h.cm \
	libupnp.pc.in \
	libupnp.spec \
	.gitignore

# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir = $(libdir)/pkgconfig
pkgconfigexec_DATA = libupnp.pc

$(pkgconfigexec_DATA): config.status

CLEANFILES = build/inc/autoconfig.h build/inc/upnpconfig.h
