##
 #  ummd ( Micro MultiMedia Daemon )
 ##

SUBDIRS = filters io

noinst_LTLIBRARIES = libcore.la

libcore_la_LIBADD = \
	filters/libfilters.la \
	io/libio.la

libcore_la_SOURCES = \
	main.c \
	ports.c \
	wirings.c
