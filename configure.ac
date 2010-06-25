##
 #  ummd ( Micro MultiMedia Daemon )
 ##

AC_PREREQ(2.57)

AC_INIT(ummd, 0.1)

AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_HEADER(include/autoconf.h)
AC_CONFIG_SRCDIR(src/main.c)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(1.7)

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_LIBTOOL
AC_PROG_INSTALL

PKG_PROG_PKG_CONFIG

MY_CFLAGS=""
MY_LIBS=""

MY_CFG_DIR="${sysconfdir}"
MY_LOG_DIR="${localstatedir}/log"
MY_RUN_DIR="${localstatedir}/run"

AC_ARG_ENABLE(
	[debug],
	[AC_HELP_STRING([--enable-debug], [enable debugging support [default=no]])],
	[enable_debug=$enableval],
	[enable_debug='no']
)
if test "x$enable_debug" = "xyes"; then
	AC_DEFINE([MY_DEBUGGING], [1], [ Define to 1 if debugging support is enabled. ])
fi

AC_CHECK_LIB(
	[pthread],
	[pthread_create],
	[MY_LIBS="$MY_LIBS -lpthread"]
)

AC_CHECK_LIB(
	[rt],
	[clock_gettime],
	[libevent_extra_libs="-lrt"],
	[libevent_extra_libs=""]
)

AC_CHECK_HEADER(
	[event.h],
	[libevent_headers_found=yes],
	[libevent_headers_found=no]
)
AC_CHECK_LIB(
	[event],
	[event_init],
	[libevent_found=yes],
	[libevent_found=no],
	[$libevent_extra_libs]
)
if test "x$libevent_headers_found" = "xyes" -a "x$libevent_found" = "xyes" ; then
	LIBEVENT_CFLAGS=""
	LIBEVENT_LIBS="-levent $libevent_extra_libs"
else
	AC_MSG_ERROR([libevent not found])
fi
AC_ARG_VAR([LIBEVENT_CFLAGS], [C compiler flags for LIBEVENT])
AC_ARG_VAR([LIBEVENT_LIBS], [linker flags for LIBEVENT])

PKG_CHECK_MODULES(LIBAVCODEC, libavcodec >= 52)
PKG_CHECK_MODULES(LIBAVDEVICE, libavdevice >= 52)
PKG_CHECK_MODULES(LIBAVFORMAT, libavformat >= 52)
PKG_CHECK_MODULES(LIBAVUTIL, libavutil >= 49)

PKG_CHECK_MODULES(LIBCONFIG, libconfig >= 1.0)

PKG_CHECK_MODULES(LIBLO, liblo >= 0.26)

AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	include/Makefile
	src/Makefile
	src/conf/Makefile
	src/core/Makefile
	src/core/controls/Makefile
	src/core/filters/Makefile
	src/core/sources/Makefile
	src/core/targets/Makefile
	src/util/Makefile
])

AC_SUBST(MY_DEBUGGING)

AC_SUBST(MY_CFLAGS)
AC_SUBST(MY_LIBS)

AC_SUBST(MY_CFG_DIR)
AC_SUBST(MY_LOG_DIR)
AC_SUBST(MY_RUN_DIR)
AC_OUTPUT
