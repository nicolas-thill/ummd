##
 #  ummd ( Micro MultiMedia Daemon )
 ##

SUBDIRS = controls filters io

noinst_LTLIBRARIES = libcore.la

libcore_la_LIBADD = \
	controls/libcontrols.la \
	filters/libfilters.la \
	io/libio.la

libcore_la_SOURCES = \
	main.c \
	ports.c \
	wirings.c
