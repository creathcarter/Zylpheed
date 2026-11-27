#!/bin/bash
#
# build-all-win64.sh - build Zylpheed and all three Windows packages.
#
#   nsis/build-all-win64.sh [OUT_DIR]
#
#   OUT_DIR  where the packages are written (default: ../snapshots beside
#            the source tree)
#
# Run in the MSYS2 UCRT64 shell. build-win64/ must be configured already
# (BUILD-WIN64.md). Steps:
#   1. make and make install in build-win64/
#   2. OUT_DIR/zylpheed-portable/            portable folder (with run-test.bat)
#   3. OUT_DIR/Zylpheed-<ver>-x64_setup.exe  installer
#   4. OUT_DIR/Zylpheed-<ver>-x64_portable.exe  single-file portable program
#
# The portable folder is first made as OUT_DIR/zylpheed-portable.new and
# then put in place of the old one. The profile of the old folder
# (profiles/, or profile/ from older builds) is carried over. If the old
# folder is in use (Zylpheed is running from it), it is left alone and the
# new folder stays as zylpheed-portable.new.

set -e
set -o pipefail

TOP=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-$TOP/../snapshots}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
DIST=$OUT/zylpheed-portable
NEW=$DIST.new
OLD=$DIST.old
[ -f "$TOP/build-win64/Makefile" ] || { echo "build-win64 is not configured" >&2; exit 1; }

echo "== make"
make -C "$TOP/build-win64" -j8 2>&1 | grep -E "error|\*\*\*" || true
make -C "$TOP/build-win64" install >/dev/null

echo "== portable folder"
rm -rf "$NEW" "$OLD"
TEST_LAUNCHER=1 sh "$TOP/nsis/make-dist-win64.sh" "$TOP/build-win64" "$NEW" | tail -2

echo "== installer"
sh "$TOP/nsis/make-installer-win64.sh" "$NEW" "$OUT" | grep -E "^Output|uninstall list|rror" || true

echo "== portable .exe"
sh "$TOP/nsis/make-portable-win64.sh" "$NEW" "$OUT" | grep -E "^Output|rror" || true

echo "== put the portable folder in place"
if [ ! -d "$DIST" ]; then
	mv "$NEW" "$DIST"
elif mv "$DIST" "$OLD" 2>/dev/null; then
	mv "$NEW" "$DIST"
	if [ -d "$OLD/profiles" ]; then
		mv "$OLD/profiles" "$DIST/profiles"
	elif [ -d "$OLD/profile" ]; then
		# older builds: run-test.bat kept the profile in profile/
		mv "$OLD/profile" "$DIST/profiles"
	fi
	rm -rf "$OLD"
	echo "replaced $DIST"
else
	echo "NOTE: $DIST is in use (is Zylpheed running from it?)."
	echo "      It was not changed. The new folder is $NEW"
fi

ls -l "$OUT"/Zylpheed-*-x64_*.exe
