if ACCEL_X86

INCLUDES = -I$(top_srcdir)/src

AM_CXXFLAGS = -ansi -pedantic -Wall -Wno-long-long -Wno-unknown-pragmas
AM_CFLAGS = -std=c99

SUFFIXES = .asm
AS = yasm
ASFLAGS = -f elf -m x86 -prefer-non-pic

.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile $(AS) $(ASFLAGS) $(AM_ASFLAGS) -o $@ $<

noinst_LTLIBRARIES = libCapseoAccel.la

ARCH_GENERIC = $(top_srcdir)/src/arch-generic

EXTRA_DIST = bgra2yuv420.asm
libCapseoAccel_la_SOURCES = \
	$(ARCH_GENERIC)/bgra2yuv420.c \
	scale.asm

endif

# vim:ai:noet:ts=4:nowrap
