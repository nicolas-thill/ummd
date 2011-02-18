##
 #  ummd ( Micro MultiMedia Daemon )
 ##

noinst_LTLIBRARIES = libtargets.la

libtargets_la_SOURCES = \
	all.c \
	file.c \
	udp.c

if HAVE_OSS
  libtargets_la_SOURCES += oss.c
  libtargets_la_LDFLAGS = @OSS_LDFLAGS@
  libtargets_la_CFLAGS  = @OSS_CFLAGS@
  libtargets_la_LIBADD  = @OSS_LIBS@ 
endif
