# -*- Autoconf -*-
#
# Top-level configure.ac file for libupnp
#
# Process this file with autoconf to produce a configure script.
#
# (C) Copyright 2005-2007 Rémi Turboult <r3mi@users.sourceforge.net>
#

AC_PREREQ(2.60)

AC_INIT([libupnp], [1.14.1], [mroberto@users.sourceforge.net])
dnl ############################################################################
dnl # *Independently* of the above libupnp package version, the libtool version
dnl # of the 3 libraries need to be updated whenever there is a change released:
dnl # "current:revision:age" (this is NOT the same as the package version),
dnl # where:
dnl # 	- library code modified:		revision++
dnl # 	- interfaces changed/added/removed:	current++ and revision=0
dnl # 	- interfaces added: 			age++
dnl # 	- interfaces removed or changed:	age=0
dnl # *please update only once, before a formal release, not for each change*
dnl #
dnl ############################################################################
dnl # Release 1.12.1
dnl # "current:revision:age"
dnl #
dnl # - Code has changed in upnp
dnl #   revision: 0 -> 1
dnl #
dnl #AC_SUBST([LT_VERSION_IXML],       [11:0:0])
dnl #AC_SUBST([LT_VERSION_UPNP],       [16:1:0])
dnl #
dnl ############################################################################
dnl # Release 1.14.0
dnl # "current:revision:age"
dnl #
dnl # - Code has changed in upnp
dnl #   revision: 1 -> 2
dnl # - Interfaces removed in upnp
dnl #   current: 16 -> 17
dnl #   revision: 2 -> 0
dnl # - Interfaces removed in upnp
dnl #   age: 0 -> 0
dnl #
dnl #AC_SUBST([LT_VERSION_IXML],       [11:0:0])
dnl #AC_SUBST([LT_VERSION_UPNP],       [17:0:0])
dnl #
dnl ############################################################################
dnl # Release 1.14.1
dnl # "current:revision:age"
dnl #
dnl # - Code has changed in upnp
dnl #   revision: 0 -> 1
dnl #
dnl #AC_SUBST([LT_VERSION_IXML],       [11:0:0])
dnl #AC_SUBST([LT_VERSION_UPNP],       [17:1:0])
dnl #
dnl ############################################################################
AC_SUBST([LT_VERSION_IXML],       [11:0:0])
AC_SUBST([LT_VERSION_UPNP],       [17:1:0])
dnl ############################################################################
dnl # Repeating the algorithm to place it closer to the modificatin place:
dnl # 	- library code modified:		revision++
dnl # 	- interfaces changed/added/removed:	current++ and revision=0
dnl # 	- interfaces added: 			age++
dnl # 	- interfaces removed or changed:	age=0
dnl # *please update only once, before a formal release, not for each change*
dnl ############################################################################


AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([upnp/inc/upnp.h])


AM_INIT_AUTOMAKE([1.8 -Wall foreign subdir-objects dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


#
# Get canonical host names in host and host_os
#
AC_CANONICAL_HOST

#
# There are 3 configuration files :
# 1) "./autoconfig.h" is auto-generated and used only internally during build
#    (usually named "config.h" but conflicts with the file below)
# 2) "./upnp/src/inc/config.h" is static and contains some compile-time 
#    parameters. This file was previously in "./upnp/inc" but is no longer 
#    installed (contains internal definitions only).
# 3) "./upnp/inc/upnpconfig.h" is auto-generated and installed with the 
#    libraries : it contains information on the configuration of the
#    installed libraries.
#
AC_CONFIG_HEADERS([autoconfig.h upnp/inc/upnpconfig.h upnp/sample/common/config_sample.h])
AX_SYS_LARGEFILE_SENSITIVE

if test "x$ac_cv_sys_largefile_sensitive" = "xyes"; then
	AC_DEFINE([UPNP_LARGEFILE_SENSITIVE], 1, [whether the system defaults to 32bit off_t but can do 64bit when requested])
fi

upnpmaj=`echo "$PACKAGE_VERSION" | sed 's/\..*//' `
upnpmin=[`echo "$PACKAGE_VERSION" | sed 's/^[^.]\.// ; s/[^0-9].*$//' `]
upnppatch=[`echo "$PACKAGE_VERSION" | sed 's/^[^\.]*\.[^.]*\.//' `]
AC_DEFINE_UNQUOTED([UPNP_VERSION_STRING], "$PACKAGE_VERSION", [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_MAJOR], $upnpmaj, [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_MINOR], $upnpmin, [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_PATCH], $upnppatch, [see upnpconfig.h])


#
# Check for DEBUG flag
#
RT_BOOL_ARG_ENABLE([debug], [no], [extra debugging code])
if test "x$enable_debug" = xyes ; then
	AC_DEFINE(UPNP_HAVE_DEBUG, 1, [see upnpconfig.h])
	AC_DEFINE(DEBUG, 1, [Define to 1 to compile debug code])
else
	AC_DEFINE(NDEBUG, 1, [Define to 1 to prevent compilation of assert()])
fi


#
# Check for libupnp subsets
#
RT_BOOL_ARG_ENABLE([client], [yes], [control point code (client)])
if test "x$enable_client" = xyes ; then
	AC_DEFINE(UPNP_HAVE_CLIENT, 1, [see upnpconfig.h])
fi


RT_BOOL_ARG_ENABLE([device], [yes],
     [device specific code (implies --disable-webserver if disabled)])
if test "x$enable_device" = xyes ; then
	AC_DEFINE(UPNP_HAVE_DEVICE, 1, [see upnpconfig.h])
fi


if test "x$enable_device" = xno ; then 
	enable_webserver=no
else 
	RT_BOOL_ARG_ENABLE([webserver], [yes], [integrated web server])
fi
AM_CONDITIONAL(ENABLE_WEBSERVER, test x"$enable_webserver" = xyes)
if test "x$enable_webserver" = xyes ; then
	AC_DEFINE(UPNP_HAVE_WEBSERVER, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([ssdp], [yes], [SSDP part])
if test "x$enable_ssdp" = xyes ; then
        AC_DEFINE(UPNP_HAVE_SSDP, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([optssdp], [yes], [optional SSDP headers support)])
if test "x$enable_optssdp" = xyes ; then
        AC_DEFINE(UPNP_HAVE_OPTSSDP, 1, [see upnpconfig.h])
	enable_uuid=yes
fi

RT_BOOL_ARG_ENABLE([soap], [yes], [SOAP part])
if test "x$enable_soap" = xyes ; then
        AC_DEFINE(UPNP_HAVE_SOAP, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([gena], [yes], [GENA part])
if test "x$enable_gena" = xyes ; then
        AC_DEFINE(UPNP_HAVE_GENA, 1, [see upnpconfig.h])
	enable_uuid=yes
fi

AM_CONDITIONAL(ENABLE_UUID, test x"$enable_uuid" = xyes)

RT_BOOL_ARG_ENABLE([tools], [yes], [helper APIs in upnptools.h])
if test "x$enable_tools" = xyes ; then
	AC_DEFINE(UPNP_HAVE_TOOLS, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([ipv6], [yes], [ipv6 support])
if test "x$enable_ipv6" = xyes ; then
        AC_DEFINE(UPNP_ENABLE_IPV6, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([unspecified_server], [no], [unspecified SERVER header])
if test "x$enable_unspecified_server" = xyes ; then
        AC_DEFINE(UPNP_ENABLE_UNSPECIFIED_SERVER, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([open_ssl], [no], [open-ssl support])
if test "x$enable_open_ssl" = xyes ; then
        AC_DEFINE(UPNP_ENABLE_OPEN_SSL, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([blocking_tcp_connections], [yes], [blocking TCP connections])
if test "x$enable_blocking_tcp_connections" = xyes ; then
        AC_DEFINE(UPNP_ENABLE_BLOCKING_TCP_CONNECTIONS, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([scriptsupport], [yes], [script support for IXML document tree, see ixml.h])
if test "x$enable_scriptsupport" = xyes ; then
        AC_DEFINE(IXML_HAVE_SCRIPTSUPPORT, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([postwrite], [no], [write to the filesystem on otherwise unhandled POST requests])
if test "x$enable_postwrite" = xyes ; then
        AC_DEFINE(UPNP_ENABLE_POST_WRITE, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([reuseaddr], [no], [bind the miniserver socket with reuseaddr to allow clean restarts])
if test "x$enable_reuseaddr" = xyes ; then
        AC_DEFINE(UPNP_MINISERVER_REUSEADDR, 1, [see upnpconfig.h])
fi

RT_BOOL_ARG_ENABLE([samples], [yes], [compilation of upnp/sample/ code])

#
# Checks for programs
#
AC_PROG_CC
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_EGREP

#
# Default compilation flags
#
echo "--------------------- Default compilation flags -------------------------------"
echo host is $host
echo host_os is $host_os

build_linux=no
build_windows=no
build_mac=no
build_freebsd=no

# Detect the target system
case "${host_os}" in
linux*)
        build_linux=yes
	AC_MSG_NOTICE([Building for Linux])
	;;
cygwin*|mingw*)
        build_windows=yes
	AC_MSG_NOTICE([Building for Windows])
	;;
darwin*)
        build_mac=yes
	AC_MSG_NOTICE([Building for Mac])
	;;
freebsd*)
        build_freebsd=yes
	AC_MSG_NOTICE([Building for FreeBSD])
	;;
#*)
#        AC_MSG_ERROR(["OS $host_os is not supported"])
#	;;
esac

# Pass the conditionals to automake
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])
AM_CONDITIONAL([FREEBSD], [test "$build_freebsd" = "yes"])

################################################################################

case $host_os in
freebsd*)
	echo "Using FreeBSD specific compiler settings"
	# Put FreeBSD specific compiler flags here
	;;
*)
	echo "Using non-specific system compiler settings"
	if test x"$enable_debug" = xyes; then
		# AC_PROG_CC already sets CFLAGS to "-g -O2" by default,
		# but only if CFLAGS was not previously set.
		#:
		# Use -O0 in debug so that variables do not get optimized out
		AX_CFLAGS_GCC_OPTION([-O0, -g])
	else
		# add optimise for size
		AX_CFLAGS_GCC_OPTION([-Os])
	fi
	;;
esac

AX_CFLAGS_WARN_ALL
AX_CFLAGS_GCC_OPTION([-Wextra -Wpedantic])

echo "-------------------------------------------------------------------------------"


AC_TYPE_SIZE_T


#
# Are we targetting Win32?
#
AC_MSG_CHECKING([for Win32])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#ifdef _WIN32
	This is an error!
#endif
],[])], [ac_cv_win32="no"], [ac_cv_win32="yes"])
if test "$ac_cv_win32" = "yes"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AM_CONDITIONAL(WIN32, test x"$ac_cv_win32" = xyes)


#
# Checks for header files (which aren't needed on Win32)
#
AC_HEADER_STDC
if test "$ac_cv_win32" = "no"; then
# libupnp code doesn't use autoconf variables yet,
# so just abort if a header file is not found.
AC_CHECK_HEADERS(
	[ \
	arpa/inet.h \
	fcntl.h \
	inttypes.h \
	limits.h \
	netdb.h \
	netinet/in.h \
	stdlib.h \
	string.h \
	sys/ioctl.h \
	sys/socket.h \
	sys/time.h \
	syslog.h \
	unistd.h \
	],
	[],
	[AC_MSG_ERROR([required header file missing])])
fi


#
# Checks for typedefs, structures, and compiler characteristics
#	
AC_C_CONST
AC_C_BIGENDIAN

AC_CHECK_HEADERS([sys/types.h sys/socket.h ws2tcpip.h])
AC_MSG_CHECKING(for socklen_t)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif
#ifdef HAVE_WS2TCPIP_H
	#include <ws2tcpip.h>
#endif
],[ socklen_t t = 0; return t; ])
],[ac_cv_socklen_t="yes"],[ac_cv_socklen_t="no"])
if test "$ac_cv_socklen_t" = "yes"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no, using int])
	AC_DEFINE(socklen_t, int, [Type for storing the length of struct sockaddr])
fi


#
# Checks for library functions
#
AC_FUNC_VPRINTF
AC_FUNC_FSEEKO
AC_CHECK_FUNC(strnlen,
	AC_DEFINE(HAVE_STRNLEN, 1, [Defines if strnlen is available on your system]))
AC_CHECK_FUNC(strndup,
	AC_DEFINE(HAVE_STRNDUP, 1, [Defines if strndup is available on your system]))
#
# Solaris needs -lsocket -lnsl -lrt
AC_SEARCH_LIBS([bind],           [socket])
AC_SEARCH_LIBS([gethostbyname],  [nsl])
AC_SEARCH_LIBS([sched_getparam], [rt])


#
# Checks for POSIX Threads
#
echo "--------------------------- pthread stuff -------------------------------------"
AX_PTHREAD(
	[],
	[AC_MSG_ERROR([POSIX threads are required to build this program])])
#
# Update environment variables for pthreads
#
CC="$PTHREAD_CC"
CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
LIBS="$PTHREAD_LIBS $LIBS"
#
# Determine if pthread_rwlock_t is available
#
echo "----------------------- pthread_rwlock_t stuff --------------------------------"
AC_MSG_CHECKING([if pthread_rwlock_t is available])
AC_LANG([C])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[#include <pthread.h>],
		[pthread_rwlock_t *x;])],
	[AC_DEFINE([UPNP_USE_RWLOCK], [1], [Use pthread_rwlock_t])
		AC_MSG_RESULT([yes, supported without any options])],
	[AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
			[#define _GNU_SOURCE
			#include <pthread.h>],
			[pthread_rwlock_t *x;])],
		[AC_DEFINE([UPNP_USE_RWLOCK], [1], [Use pthread_rwlock_t])
			CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
			AC_MSG_RESULT([yes, definition of _GNU_SOURCE required])],
		[AC_DEFINE([UPNP_USE_RWLOCK], [0], [Do not use pthread_rwlock_t])
			AC_MSG_RESULT([no, needs to fallback to pthread_mutex])
			AC_MSG_ERROR([pthread_rwlock_t not available])])])
echo "-------------------------------------------------------------------------------"

if test "x$enable_open_ssl" = xyes ; then
	PKG_CHECK_MODULES(OPENSSL, libssl,
		[LIBS="$LIBS $OPENSSL_LIBS" CFLAGS="$CFLAGS $OPENSSL_CFLAGS"],
		[AC_MSG_ERROR([openssl not found])])
fi

AC_CONFIG_FILES([
	Makefile
	docs/Makefile
	ixml/Makefile
	upnp/Makefile
	upnp/sample/Makefile
	libupnp.pc
	])

AC_OUTPUT
