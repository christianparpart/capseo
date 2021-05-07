#! /bin/sh

if [ "$1" = "clean" ] || [ "$1" = "-clean" ] || [ "$1" = "--clean" ]; then
  rm -f Makefile aclocal.m4 config.cache config.guess config.h config.h.in config.log config.status config.sub configure install-sh libtool ltconfig ltmain.sh missing mkinstalldirs stamp-h stamp-h.in depcomp stamp-h1 *~
  find . \( -name '*.o' -o -name '*.lo' -o -name 'Makefile.in' -o -name 'Makefile' -o -name '.libs' -o -name '.deps' \) -exec rm -rf {} \; &> /dev/null
  rm -rf autom4te*cache
  exit 0
fi

#export WANT_AUTOCONF="2.5"
#export WANT_AUTOMAKE="1.7"

autoreconf -f -v --install || exit 1

echo "Don't forget the standard procudure:"
echo "example: ./configure --prefix=/usr && make && make install"
