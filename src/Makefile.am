#
#  UMMD - Micro MultiMedia Daemon
#
#  Copyright (C) 2010 Nicolas Thill <nicolas.thill@gmail.com>
#

SUBDIRS = conf core util

bin_PROGRAMS = ummd

ummd_CFLAGS = \
	@LIBAVCODEC_CFLAGS@ \
	@LIBAVDEVICE_CFLAGS@ \
	@LIBAVFORMAT_CFLAGS@ \
	@LIBAVUTIL_CFLAGS@ \
	@LIBMPG123_CFLAGS@ \
	-DMY_CFG_DIR=\"@MY_CFG_DIR@\" \
	-DMY_LOG_DIR=\"@MY_LOG_DIR@\" \
	-DMY_RUN_DIR=\"@MY_RUN_DIR@\"

ummd_LDADD = \
	@LIBAVCODEC_LIBS@ \
	@LIBAVDEVICE_LIBS@ \
	@LIBAVFORMAT_LIBS@ \
	@LIBAVUTIL_LIBS@ \
	@LIBMPG123_LIBS@ \
	conf/libconf.la \
	core/libcore.la \
	util/libutil.la

ummd_SOURCES = \
	main.c
