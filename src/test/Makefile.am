##
 #  ummd ( Micro MultiMedia Daemon )
 ##

bin_PROGRAMS = fftest

fftest_CFLAGS = \
	@LIBAVCODEC_CFLAGS@ \
	@LIBAVDEVICE_CFLAGS@ \
	@LIBAVFORMAT_CFLAGS@ \
	@LIBAVUTIL_CFLAGS@

fftest_LDADD = \
	@LIBAVCODEC_LIBS@ \
	@LIBAVDEVICE_LIBS@ \
	@LIBAVFORMAT_LIBS@ \
	@LIBAVUTIL_LIBS@ \
	../util/libutil.la

fftest_SOURCES = \
	fftest.c

