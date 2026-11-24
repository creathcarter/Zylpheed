#!/bin/sh

ACLOCAL=${ACLOCAL:-aclocal}
AUTOMAKE=${AUTOMAKE:-automake}

$ACLOCAL -I ac \
  && libtoolize --force --copy \
  && autoheader \
  && $AUTOMAKE --add-missing --foreign --copy \
  && autoconf \
  && if test -z "$NOCONFIGURE"; then ./configure "$@"; fi
