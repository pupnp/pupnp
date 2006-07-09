# -*- Autoconf -*-
# $Id: configure.ac,v 1.11 2006/04/09 13:59:44 r3mi Exp $
#
# Top-level configure.ac file for libupnp
#
# Process this file with autoconf to produce a configure script.
#
# (C) Copyright 2005-2006 Rémi Turboult <r3mi@users.sourceforge.net>
#

AC_PREREQ(2.59)

AC_INIT([libupnp], [1.4.1], [virtual_worlds@gmx.de])
# *Independently* of the above libupnp package version, the libtool version
# of the 3 libraries need to be updated whenever there is a change released :
# "current:revision:age" (this is NOT the same as the package version), where:
# 	- library code modified:		revision++
# 	- interfaces changed/added/removed:	current++ and revision=0
# 	- interfaces added: 			age++
# 	- interfaces removed:			age=0
# *please update only once, before a formal release, not for each change*
AC_SUBST([LT_VERSION_IXML],       [2:2:0])
AC_SUBST([LT_VERSION_THREADUTIL], [2:2:0])
AC_SUBST([LT_VERSION_UPNP],       [2:2:0])


AC_CONFIG_AUX_DIR(config.aux)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_SRCDIR(upnp/inc/upnp.h)

AM_INIT_AUTOMAKE([1.8 -Wall foreign subdir-objects])

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
AC_CONFIG_HEADERS([autoconfig.h upnp/inc/upnpconfig.h])

AC_REVISION([$Revision: 1.11 $])

upnpmaj=`echo "$PACKAGE_VERSION" | sed 's/\..*//' `
upnpmin=[`echo "$PACKAGE_VERSION" | sed 's/^[^.]\.// ; s/[^0-9].*$//' `]
upnppatch=[`echo "$PACKAGE_VERSION" | sed 's/^[^.]\.[^.]\.// ; s/[^0-9].*$//' `]
AC_DEFINE_UNQUOTED([UPNP_VERSION_STRING], "$PACKAGE_VERSION",
	[see upnpconfig.h])
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
	AC_DEFINE(NO_DEBUG, 1, [Define to 1 to prevent some debug code])
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


RT_BOOL_ARG_ENABLE([tools], [yes], [helper APIs in upnptools.h])
if test "x$enable_tools" = xyes ; then
	AC_DEFINE(UPNP_HAVE_TOOLS, 1, [see upnpconfig.h])
fi


RT_BOOL_ARG_ENABLE([samples], [yes], [compilation of upnp/sample/ code])


# doc installation
AC_MSG_CHECKING([documentation installation])
AC_ARG_WITH([docdir], 
AC_HELP_STRING([--with-docdir=DIR],
       	       [where documentation is installed 
	        @<:@[DATADIR/doc/]AC_PACKAGE_NAME[-]AC_PACKAGE_VERSION@:>@])
AC_HELP_STRING([--without-docdir],
       	       [do not install the documentation]),
	[DOCDIR="$with_docdir"],
	[DOCDIR="${datadir}/doc/${PACKAGE_NAME}-${PACKAGE_VERSION}"])

AM_CONDITIONAL(WITH_DOCDIR, test x"$with_docdir" != xno)
AC_SUBST(DOCDIR)
AC_MSG_RESULT($DOCDIR)


#
# Checks for programs
#
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_EGREP


#
# Default compilation flags
#
if test x"$enable_debug" = xyes; then
	# AC_PROG_CC already sets CFLAGS to "-g -O2" by default
	:
else
	# add optimise for size
	AX_CFLAGS_GCC_OPTION([-Os])
fi
AX_CFLAGS_WARN_ALL

# Arrange for large-file support (can be disabled with --disable-largefile).
# Define _FILE_OFFSET_BITS and _LARGE_FILES if necessary
AC_SYS_LARGEFILE


#
# Checks for header files
#
AC_HEADER_STDC
# libupnp code doesn't use autoconf variables yet,
# so just abort if a header file is not found.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h syslog.h unistd.h],[],[AC_MSG_ERROR([required header file missing])])


#
# Checks for typedefs, structures, and compiler characteristics
#	
AC_C_CONST
AC_TYPE_SIZE_T
TYPE_SOCKLEN_T


#
# Checks for library functions
#
AC_FUNC_VPRINTF
AC_FUNC_FSEEKO


#
# Checks for POSIX Threads
#
ACX_PTHREAD([],[AC_MSG_ERROR([POSIX threads are required to build this program])])




AC_CONFIG_FILES([Makefile
		 ixml/Makefile
		 ixml/doc/Makefile
                 threadutil/Makefile
		 upnp/Makefile
		 upnp/doc/Makefile
		 docs/dist/Makefile
		 libupnp.pc
                 ])

AC_OUTPUT

