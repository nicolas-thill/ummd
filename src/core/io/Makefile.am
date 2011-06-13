##
 #  ummd ( Micro MultiMedia Daemon )
 ##

noinst_LTLIBRARIES = libio.la

libio_la_CFLAGS  =
libio_la_LDFLAGS =
libio_la_LIBADD  =
libio_la_SOURCES = \
	all.c \
	fifo.c \
	file.c \
	sock.c \
	udp.c

if HAVE_OSC
  libio_la_CFLAGS  += @LIBLO_CFLAGS@
  libio_la_LDFLAGS += @LIBLO_LDFLAGS@
  libio_la_LIBADD  += @LIBLO_LIBS@
  libio_la_SOURCES += osc.c
endif

if HAVE_OSS
  libio_la_CFLAGS  += @OSS_CFLAGS@
  libio_la_LDFLAGS += @OSS_LDFLAGS@
  libio_la_LIBADD  += @OSS_LIBS@
  libio_la_SOURCES += oss.c
endif
