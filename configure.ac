AC_INIT(src/capseo.h)

CAPSEO_MAJOR_VERSION=0
CAPSEO_MINOR_VERSION=2
CAPSEO_MICRO_VERSION=0
CAPSEO_RELEASE_INFO="-dev" # ^^ set to "" for releases - otherwise to "-dev"

CAPSEO_VERSION=$CAPSEO_MAJOR_VERSION.$CAPSEO_MINOR_VERSION.$CAPSEO_MICRO_VERSION$CAPSEO_RELEASE_INFO
CAPSEO_VERSION_INFO=`expr $CAPSEO_MAJOR_VERSION + $CAPSEO_MINOR_VERSION`:$CAPSEO_MICRO_VERSION:$CAPSEO_MINOR_VERSION
CAPSEO_VERSION_NUMBER=`expr $CAPSEO_MAJOR_VERSION \* 10000 + $CAPSEO_MINOR_VERSION \* 100 + $CAPSEO_MICRO_VERSION`

AC_SUBST(CAPSEO_MAJOR_VERSION)
AC_SUBST(CAPSEO_MINOR_VERSION)
AC_SUBST(CAPSEO_MICRO_VERSION)
AC_SUBST(CAPSEO_VERSION)
AC_SUBST(CAPSEO_VERSION_INFO)
AC_SUBST(CAPSEO_VERSION_NUMBER)

VERSION=${CAPSEO_VERSION}

AM_INIT_AUTOMAKE(capseo, $VERSION)

AM_CONFIG_HEADER(config.h)

AC_GNU_SOURCE
AC_ISC_POSIX

AC_PROG_INSTALL
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AM_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG([0.17.2])

dnl {{{ --with-accel=ARCH
AC_ARG_WITH([accel], [
  --with-accel=PATH          Specifies the architecture the code shall be 
                             accelerated for (generic,amd64,x86)],
  [with_accel=${withval}],
  [with_accel=generic]
)
ACCEL=$with_accel
AC_SUBST(ACCEL)
AM_CONDITIONAL([ACCEL_GENERIC], [test x$with_accel = xgeneric])
AM_CONDITIONAL([ACCEL_AMD64], [test x$with_accel = xamd64])
AM_CONDITIONAL([ACCEL_X86], [test x$with_accel = xx86])
dnl }}}

dnl {{{ --enable-debug
AC_ARG_ENABLE([debug], [
  --enable-debug          Enables debug build(s)],
  [enable_debug=${withval}],
  [enable_debug=yes]
)
if test x$enable_debug = xyes; then
    DEBUG_CXXFLAGS="$CXXFLAGS -O0 -ggdb3"
    DEBUG_CFLAGS="$CXXFLAGS -O0 -ggdb3"
else
    DEBUG_CXXFLAGS="$CXXFLAGS -O3 -DNDEBUG"
    DEBUG_CFLAGS="$CXXFLAGS -O3 -DNDEBUG"
fi
AC_SUBST(DEBUG_CXXFLAGS)
AC_SUBST(DEBUG_CFLAGS)
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])
dnl }}}

dnl {{{ --enable-examples
AC_ARG_ENABLE([examples], [
  --enable-examples       Enables compilation of example program(s)],
  [enable_examples=${enableval}],
  [enable_examples=yes]
)
AM_CONDITIONAL([EXAMPLE], [test x$enable_examples = xyes])
dnl }}}

dnl {{{ --enable-theora
AC_ARG_ENABLE([theora], [
  --enable-theora         Enables ogg/theora output support in cpsrecode],
  [enable_theora=${enableval}],
  [enable_theora=no]
)
if test x$enable_theora = xyes; then
  PKG_CHECK_MODULES([THEORA], [theora])
  PKG_CHECK_MODULES([OGG], [ogg >= 1.1])
  THEORA=1
else
  THEORA=0
fi
AC_SUBST([THEORA])
AM_CONDITIONAL([THEORA], [test x$enable_theora = xyes])
dnl }}}

dnl {{{ --disable-tools
AC_ARG_ENABLE([tools], [
  --disable-tools        Disable cupseo tools],
  [enable_tools=${enableval}],
  [enable_tools=yes]
)
AM_CONDITIONAL([CAPSEO_TOOLS], [test x$enable_tools = xyes])
dnl }}}

dnl -------------------------------------------------------------------------
AC_OUTPUT([
  Makefile
  capseo.pc
  capseo.spec
  src/Makefile
  src/arch-generic/Makefile
  src/arch-amd64/Makefile
  src/arch-x86/Makefile
  tools/Makefile
  examples/Makefile
])

echo "---------------------------------------------------"
echo "cpu acceleration:                  ${with_accel}"
echo "cpsrecode theora support:          ${enable_theora}"
echo "compile tools:                     ${enable_tools}"
echo "compile examples:                  ${enable_examples}"
echo

dnl vim:ai:et:ts=2:nowrap
