INCLUDES = -I$(top_srcdir)/src

AM_CXXFLAGS = -ansi -pedantic -Wall

if EXAMPLE

noinst_PROGRAMS = encode-raw encode-stream

encode_raw_SOURCES = encode-raw.c
encode_raw_LDADD = $(top_builddir)/src/libcapseo.la -lGL -lX11 -lm

encode_stream_SOURCES = encode-stream.c
encode_stream_LDADD = $(top_builddir)/src/libcapseo.la -lGL -lX11 -lm

endif

# vim:ai:noet:ts=4:nowrap
