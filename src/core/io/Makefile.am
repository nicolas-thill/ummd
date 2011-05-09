##
 #  ummd ( Micro MultiMedia Daemon )
 ##

noinst_LTLIBRARIES = libio.la

libio_la_SOURCES = \
	all.c \
	file.c \
	udp.c

if HAVE_OSS
  libio_la_SOURCES += oss.c
  libio_la_LDFLAGS = @OSS_LDFLAGS@
  libio_la_CFLAGS  = @OSS_CFLAGS@
  libio_la_LIBADD  = @OSS_LIBS@
endif
