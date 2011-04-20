#
#  UMMD - Micro MultiMedia Daemon
#
#  Copyright (C) 2010 Nicolas Thill <nicolas.thill@gmail.com>
#

SUBDIRS = conf core util test

bin_PROGRAMS = ummd

ummd_CFLAGS = \
	@LIBMPG123_CFLAGS@ \
	-DMY_CFG_DIR=\"@MY_CFG_DIR@\" \
	-DMY_LOG_DIR=\"@MY_LOG_DIR@\" \
	-DMY_RUN_DIR=\"@MY_RUN_DIR@\"

ummd_LDADD = \
	@LIBMPG123_LIBS@ \
	conf/libconf.la \
	core/libcore.la \
	util/libutil.la

ummd_SOURCES = \
	main.c
