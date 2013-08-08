#
# XWiimote - configure
# Written by David Herrmann, 2011
# Dedicated to the Public Domain
#

AC_PREREQ(2.68)

AC_INIT([wheeliemote],
        [2],
        [http://github.com/dvdhrm/xwiimote/issues],
        [wheeliemote],
        [http://dvdhrm.github.io/xwiimote])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(config.h)
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects dist-xz no-dist-gzip tar-pax -Wall -Werror -Wno-portability])
AM_SILENT_RULES([yes])

# Don't add a default "-g -O2" if CFLAGS wasn't specified
: ${CFLAGS=""}

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_PREREQ(2.2)
LT_INIT

PKG_CHECK_MODULES([UDEV], [libudev])
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([BLUEZ], [bluez])
PKG_CHECK_MODULES([LIBXWIIMOTE], [libxwiimote])
AC_SUBST(UDEV_CFLAGS)
AC_SUBST(UDEV_LIBS)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
AC_SUBST(BLUEZ_CFLAGS)
AC_SUBST(BLUEZ_LIBS)

AC_MSG_CHECKING([whether to build with debugging on])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug], [whether to build with debugging on)])],
              [debug="$enableval"],
              [debug=no; AC_DEFINE([NDEBUG], [1], [No Debug])])
AM_CONDITIONAL([DEBUG], [test x$debug = xyes])
AC_MSG_RESULT([$debug])

#
# Check for doxygen
#

#
# Write configuration
#

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([Build configuration:

               prefix: $prefix
          exec-prefix: $exec_prefix
               libdir: $libdir
           includedir: $includedir

                debug: $debug

        Run "${MAKE-make}" to start compilation process])
