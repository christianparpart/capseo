SUBDIRS = arch-generic arch-amd64 arch-x86

INSTALL_HEADER = $(INSTALL_DATA) -p

INCLUDES = -I$(top_srcdir)/src

QUICKLZ_FLAGS = -DQLZ_MEMORY_SAFE=1 -DQLZ_COMPRESSION_LEVEL=1 -DQLZ_STREAMING_BUFFER=0
AM_CPPFLAGS = $(QUICKLZ_FLAGS)

AM_CXXFLAGS = -ansi -pedantic -Wall -Wno-long-long -Wno-unknown-pragmas
AM_CFLAGS = -std=c99

lib_LTLIBRARIES = libcapseo.la

libcapseo_la_LDFLAGS = -version-info $(CAPSEO_VERSION_INFO) -lm

libcapseo_la_SOURCES = \
	quicklz.c quicklz.h \
	compress.h compress_quicklz.cpp \
	global.cpp \
	cursor.cpp \
	encode.cpp \
	decode.cpp \
	stream.cpp \
	error.cpp

libcapseo_la_LIBADD = arch-$(ACCEL)/libCapseoAccel.la

capseodir = @includedir@
capseo_HEADERS = capseo.h

# vim:ai:noet:ts=4:nowrap
