#!/bin/bash
#
# make-portable-win64.sh - build the single-file portable Zylpheed .exe from
# the folder made by make-dist-win64.sh.
#
#   nsis/make-portable-win64.sh DIST_DIR [OUT_DIR] [makensis options...]
#
#   DIST_DIR  folder made by make-dist-win64.sh
#   OUT_DIR   where Zylpheed-<version>-x64_portable.exe is written (default:
#             the parent directory of DIST_DIR)
#
# Needs makensis on PATH (MSYS2: pacman -S mingw-w64-ucrt-x86_64-nsis).
# See zylpheed-portable.nsi for what the .exe does when it is started.

set -e

NSISDIR_SRC=$(cd "$(dirname "$0")" && pwd)
[ -n "$1" ] || { echo "usage: $0 DIST_DIR [OUT_DIR] [makensis options]" >&2; exit 1; }
DIST=$(cd "$1" && pwd)
OUT=${2:-$(dirname "$DIST")}
shift; [ $# -gt 0 ] && shift
OUT=$(cd "$OUT" && pwd)
[ -f "$DIST/zylpheed.exe" ] || { echo "$DIST/zylpheed.exe not found" >&2; exit 1; }
command -v makensis >/dev/null || { echo "makensis not found" >&2; exit 1; }

cd "$NSISDIR_SRC"
# -D options with Windows paths: MSYS would rewrite a leading "/".
makensis -V2 \
	-DDIST_DIR="$(cygpath -w "$DIST")" \
	-DOUTDIR="$(cygpath -w "$OUT")" \
	-DBUILD_STAMP="$(date +%Y%m%d-%H%M%S)" \
	"$@" zylpheed-portable.nsi
