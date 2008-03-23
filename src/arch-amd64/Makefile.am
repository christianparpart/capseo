if ACCEL_AMD64

INCLUDES = -I$(top_srcdir)/src

AM_CXXFLAGS = -ansi -pedantic -Wall -Wno-long-long -Wno-unknown-pragmas
AM_CFLAGS = -std=c99

SUFFIXES = .asm
AS = yasm
ASFLAGS = -f elf -m amd64 -prefer-non-pic

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile $(AS) $(ASFLAGS) $(AM_ASFLAGS) -o $@ $<

noinst_LTLIBRARIES = libCapseoAccel.la

ARCH_GENERIC = $(top_srcdir)/src/arch-generic

libCapseoAccel_la_SOURCES = \
	bgra2yuv420.asm \
	scale.asm \
	$(ARCH_GENERIC)/encode.cpp \
	$(ARCH_GENERIC)/decode.cpp

endif

# vim:ai:noet:ts=4:nowrap
