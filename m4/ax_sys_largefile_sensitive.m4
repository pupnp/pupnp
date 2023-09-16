# ===============================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_sys_largefile_sensitive.html
# ===============================================================================
#
# SYNOPSIS
#
#   AX_SYS_LARGEFILE_SENSITIVE
#
# DESCRIPTION
#
#   Check whether the current system is sensitive to -Ddefines making off_t
#   having different types/sizes. Automatically define a config.h symbol
#   LARGEFILE_SENSITIVE if that is the case, otherwise leave everything as
#   is.
#
#   This macro builds on top of AC_SYS_LARGEFILE to detect whether special
#   options are needed to make the code use 64bit off_t - in many setups
#   this will also make the code use 64bit off_t immediately.
#
#   The common use of a LARGEFILE_SENSITIVE config.h-define is to rename
#   exported functions, usually adding a 64 to the original function name.
#   Such renamings are only needed on systems being both (a) 32bit off_t by
#   default and (b) implementing large.file extensions (as for unix98).
#
#   a renaming section could look like this:
#
#    #if defined LARGEFILE_SENSITIVE && _FILE_OFFSET_BITS+0 == 64
#    #define zzip_open zzip_open64
#    #define zzip_seek zzip_seek64
#    #endif
#
#   for libraries, it is best to take advantage of the prefix-config.h
#   macro, otherwise you want to export a renamed LARGEFILE_SENSITIVE in an
#   installed header file. -> see AX_PREFIX_CONFIG_H
#
# LICENSE
#
#   Copyright (c) 2008 Guido U. Draheim <guidod@gmx.de>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 11

AU_ALIAS([AC_SYS_LARGEFILE_SENSITIVE], [AX_SYS_LARGEFILE_SENSITIVE])
AC_DEFUN([AX_SYS_LARGEFILE_SENSITIVE],[dnl
AC_REQUIRE([AC_SYS_LARGEFILE])dnl
# we know about some internals of ac_sys_largefile here...
AC_MSG_CHECKING(whether system differentiates 64bit off_t by defines)
ac_cv_sys_largefile_sensitive="no"
if test ".${ac_cv_sys_file_offset_bits-no}${ac_cv_sys_large_files-no}" != ".nono"
then ac_cv_sys_largefile_sensitive="yes"
  AC_DEFINE(LARGEFILE_SENSITIVE, 1,
  [whether the system defaults to 32bit off_t but can do 64bit when requested])
fi
AC_MSG_RESULT([$ac_cv_sys_largefile_sensitive])
])
