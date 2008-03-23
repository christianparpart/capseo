if ACCEL_GENERIC

INCLUDES = -I$(top_srcdir)/src

AM_CXXFLAGS = -ansi -pedantic -Wall -Wno-long-long -Wno-unknown-pragmas
AM_CFLAGS = -std=c99

noinst_LTLIBRARIES = libCapseoAccel.la

libCapseoAccel_la_SOURCES = \
	bgra2yuv420.c \
	encode.cpp \
	decode.cpp \
	scale.cpp

endif

# vim:ai:noet:ts=4:nowrap
