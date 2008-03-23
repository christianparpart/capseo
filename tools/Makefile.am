INCLUDES = -I$(top_srcdir)/src

ANSI_CXXFLAGS = -ansi -pedantic -Wall -Wno-long-long -Wno-unknown-pragmas
AM_CXXFLAGS = $(ANSI_CXXFLAGS) $(CAPTURY_CFLAGS) -DVERSION="\"@CAPSEO_VERSION@\"" -DTHEORA=@THEORA@

if CAPSEO_TOOLS

bin_PROGRAMS = cpsinfo cpsplay cpsrecode

cpsinfo_SOURCES = cpsinfo.cpp
cpsinfo_LDFLAGS = $(top_builddir)/src/libcapseo.la

cpsplay_SOURCES = cpsplay.cpp
cpsplay_LDFLAGS = $(top_builddir)/src/libcapseo.la $(X11_LIBS) -lGL

cpsrecode_SOURCES = cpsrecode.cpp
cpsrecode_LDFLAGS = $(top_builddir)/src/libcapseo.la

if THEORA
cpsrecode_LDFLAGS += $(THEORA_LIBS) $(OGG_LIBS)
endif

endif

# vim:ai:noet:ts=4:nowrap
