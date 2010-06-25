##
 #  ummd ( Micro MultiMedia Daemon )
 ##

SUBDIRS = controls filters sources targets

noinst_LTLIBRARIES = libcore.la

libcore_la_CFLAGS = \
	@LIBEVENT_CFLAGS@

libcore_la_LIBADD = \
	controls/libcontrols.la \
	filters/libfilters.la \
	sources/libsources.la \
	targets/libtargets.la \
	@LIBEVENT_LIBS@

libcore_la_SOURCES = \
	main.c
