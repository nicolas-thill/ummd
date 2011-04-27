##
 #  ummd ( Micro MultiMedia Daemon )
 ##

bin_PROGRAMS = fftest1 fftest2

MY_CFLAGS = \
	@LIBAVCODEC_CFLAGS@ \
	@LIBAVDEVICE_CFLAGS@ \
	@LIBAVFORMAT_CFLAGS@ \
	@LIBAVUTIL_CFLAGS@

MY_LDFLAGS = \
	@LIBAVCODEC_LIBS@ \
	@LIBAVDEVICE_LIBS@ \
	@LIBAVFORMAT_LIBS@ \
	@LIBAVUTIL_LIBS@ \
	../util/libutil.la


fftest1_CFLAGS = \
	$(MY_CFLAGS)

fftest1_LDADD = \
	$(MY_LDFLAGS)

fftest1_SOURCES = \
	fftest1.c


fftest2_CFLAGS = \
	$(MY_CFLAGS)

fftest2_LDADD = \
	$(MY_LDFLAGS)

fftest2_SOURCES = \
	fftest2.c

