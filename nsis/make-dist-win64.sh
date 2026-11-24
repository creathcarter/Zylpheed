#!/bin/bash
#
# make-dist-win64.sh - assemble a self-contained, relocatable Sylpheed folder
# for 64-bit Windows from an MSYS2 UCRT64 build.
#
# Run from an MSYS2 UCRT64 shell after "make" and "make install" in the build
# directory (see BUILD-WIN64.md):
#
#   nsis/make-dist-win64.sh [BUILD_DIR [DEST_DIR]]
#
#   BUILD_DIR  configured build directory (default: build-win64 in the source
#              tree); its "make install" prefix must be BUILD_DIR/dist.
#   DEST_DIR   output folder (default: BUILD_DIR/Sylpheed). It is deleted and
#              recreated.
#
# Layout of the result (everything relative, nothing refers to MSYS2):
#
#   sylpheed.exe, syl-auth-helper.exe, every DLL they need (found by walking
#   the PE import tables recursively), gspawn-win64-helper*.exe,
#   gpgme-w32spawn.exe, curl.exe
#   plugins\                 in-tree plug-ins (attachment_tool, test)
#   etc\gtk-3.0\settings.ini GTK defaults for Windows
#   etc\ssl\certs\certs.crt  CA bundle (read by libsylph/ssl.c and
#                            syl-auth-helper.exe)
#   lib\gdk-pixbuf-2.0\      image loaders + loaders.cache (relative paths)
#   lib\enchant-2\           spell checker providers (winspell, hunspell)
#   share\glib-2.0\schemas\  compiled GSettings schemas (GTK file chooser etc.)
#   share\icons\             Adwaita, AdwaitaLegacy, hicolor (also THEMEDIR)
#   share\locale\            Sylpheed + GTK/GLib/gtkspell/iso-codes catalogs
#   share\sylpheed\          manual and FAQ
#   share\xml\iso-codes\     language/country names for gtkspell
#   doc\                     Sylpheed licence and the licences of the bundled
#                            MSYS2 packages, plus PACKAGES.txt (versions)
#
# GTK, GLib, gdk-pixbuf, enchant and fontconfig find their data relative to
# the directory of their DLL (a trailing "bin" is stripped), so DLLs live next
# to sylpheed.exe and the data directories are siblings, as in the official
# Sylpheed for Windows layout.

set -e
set -o pipefail

SRCDIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${1:-$SRCDIR/build-win64}
BUILD=$(cd "$BUILD" && pwd)
DEST=${2:-$BUILD/Sylpheed}
PREFIX=$BUILD/dist
MINGW=${MINGW_PREFIX:-/ucrt64}
OBJDUMP=$MINGW/bin/objdump.exe
SYSDIR=$(cygpath -u "${SYSTEMROOT:-C:/Windows}")/System32

die() { echo "make-dist-win64: $*" >&2; exit 1; }
note() { echo "make-dist-win64: $*"; }

[ "$MSYSTEM" = UCRT64 ] || note "warning: MSYSTEM is '$MSYSTEM', expected UCRT64"
[ -x "$PREFIX/bin/sylpheed.exe" ] || die "$PREFIX/bin/sylpheed.exe not found; run make install first"
[ -x "$OBJDUMP" ] || die "$OBJDUMP not found (install $MINGW_PACKAGE_PREFIX-binutils)"
command -v pacman >/dev/null || die "pacman not found; run from an MSYS2 shell"

note "build dir: $BUILD"
note "output:    $DEST"
rm -rf "$DEST"
mkdir -p "$DEST"

# Every file copied from $MINGW, for the licence and version lists.
MINGW_FILES=$(mktemp)
trap 'rm -f "$MINGW_FILES"' EXIT

# cp_mingw SRC_RELATIVE_TO_MINGW DEST_RELATIVE_TO_DEST
cp_mingw() {
	local src=$MINGW/$1 dst=$DEST/$2
	[ -e "$src" ] || die "missing $src"
	mkdir -p "$(dirname "$dst")"
	cp -r "$src" "$dst"
	if [ -d "$src" ]; then
		find "$src" -type f >> "$MINGW_FILES"
	else
		echo "$src" >> "$MINGW_FILES"
	fi
}

# ---------------------------------------------------------------------------
# Sylpheed itself

cp "$PREFIX/bin/sylpheed.exe" "$PREFIX/bin/syl-auth-helper.exe" "$DEST/"

# Plug-ins: "make install" does not install them; take the freshly built
# DLLs from the build tree. On Windows they are loaded from <configdir>\plugins
# and from plugins\ beside sylpheed.exe (src/main.c, plugin_init()).
mkdir -p "$DEST/plugins" "$DEST/doc/plugins"
for p in attachment_tool test; do
	dll=$BUILD/plugin/$p/.libs/$p.dll
	[ -f "$dll" ] || die "$dll not found; run make in $BUILD/plugin/$p"
	cp "$dll" "$DEST/plugins/"
	cp "$SRCDIR/plugin/$p/README" "$DEST/doc/plugins/README.$p.txt"
done

# Message catalogs, manual, FAQ (LOCALEDIR=share/locale is relative).
mkdir -p "$DEST/share"
cp -r "$PREFIX/share/locale" "$DEST/share/"
cp -r "$PREFIX/share/sylpheed" "$DEST/share/"

# Documentation and licence of Sylpheed itself.
mkdir -p "$DEST/doc"
for f in COPYING COPYING.LIB LICENSE README README.ja NEWS; do
	[ -f "$SRCDIR/$f" ] && cp "$SRCDIR/$f" "$DEST/doc/$f.txt"
done

# ---------------------------------------------------------------------------
# Helper executables that the libraries start at run time

cp_mingw bin/gspawn-win64-helper.exe gspawn-win64-helper.exe
cp_mingw bin/gspawn-win64-helper-console.exe gspawn-win64-helper-console.exe
cp_mingw bin/gpgme-w32spawn.exe gpgme-w32spawn.exe
# update_check.c runs "curl" (G_SPAWN_SEARCH_PATH) for the version check.
cp_mingw bin/curl.exe curl.exe

# ---------------------------------------------------------------------------
# Run-time modules (loaded with LoadLibrary, not in any import table)

for f in "$MINGW"/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
	cp_mingw "${f#$MINGW/}" "${f#$MINGW/}"
done
# Spell checking: winspell uses the Windows spell checker (no dictionaries to
# ship); hunspell is kept so that users can drop dictionaries into
# share\hunspell. aspell, nuspell (pulls in ICU, ~40 MB) and voikko are left out.
for p in winspell hunspell; do
	cp_mingw lib/enchant-2/enchant_$p.dll lib/enchant-2/enchant_$p.dll
done
cp_mingw share/enchant-2/enchant.ordering share/enchant-2/enchant.ordering

# ---------------------------------------------------------------------------
# Dependent DLLs: walk the import tables of every PE file in $DEST.

declare -A AVAIL      # lower-case DLL name -> source path
for f in "$MINGW"/bin/*.dll "$PREFIX"/bin/*.dll; do
	b=$(basename "$f")
	AVAIL[${b,,}]=$f
done
declare -A DONE       # lower-case DLL name -> 1 (bundled or system)
declare -A SCANNED    # PE file path -> 1
MISSING=

pe_imports() {
	"$OBJDUMP" -p "$1" | sed -n 's/^[[:space:]]*DLL Name: //p' | tr -d '\r'
}

is_system_dll() {
	local n=${1,,}
	case "$n" in
	api-ms-win-*|ext-ms-*) return 0 ;;
	esac
	[ -f "$SYSDIR/$1" ] || [ -n "$(find "$SYSDIR" -maxdepth 1 -iname "$1" -print -quit)" ]
}

while :; do
	new=0
	while IFS= read -r -d '' pe; do
		[ -n "${SCANNED[$pe]}" ] && continue
		SCANNED[$pe]=1
		new=1
		for dll in $(pe_imports "$pe"); do
			key=${dll,,}
			[ -n "${DONE[$key]}" ] && continue
			DONE[$key]=1
			if [ -n "${AVAIL[$key]}" ]; then
				src=${AVAIL[$key]}
				cp "$src" "$DEST/"
				case "$src" in
				"$MINGW"/*) echo "$src" >> "$MINGW_FILES" ;;
				esac
			elif is_system_dll "$dll"; then
				:
			else
				MISSING="$MISSING $dll(needed by ${pe#$DEST/})"
			fi
		done
	done < <(find "$DEST" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)
	[ $new = 0 ] && break
done
[ -z "$MISSING" ] || die "unresolved DLLs:$MISSING"
note "bundled $(find "$DEST" -maxdepth 1 -iname '*.dll' | wc -l) DLLs"

# Old GTK 2 libraries must never end up here.
for bad in libsylph-0-1.dll libsylpheed-plugin-0-1.dll libgtk-win32-2.0-0.dll; do
	[ ! -e "$DEST/$bad" ] || die "$bad was pulled in; stale binaries?"
done

# ---------------------------------------------------------------------------
# GTK 3 run-time data

# gdk-pixbuf loaders.cache, regenerated so that the module paths are relative
# to the folder (gdk-pixbuf writes them relative when the loaders are below
# its own installation directory). The query tool is run from inside $DEST
# with a PATH that does not contain MSYS2, then removed.
cp "$MINGW/bin/gdk-pixbuf-query-loaders.exe" "$DEST/"
(cd "$DEST" && PATH=$SYSDIR GDK_PIXBUF_MODULEDIR= GDK_PIXBUF_MODULE_FILE= \
	./gdk-pixbuf-query-loaders.exe) \
	> "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
rm -f "$DEST/gdk-pixbuf-query-loaders.exe"
grep -qi 'msys64\|ucrt64' "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" &&
	die "loaders.cache contains absolute MSYS2 paths"
n=$(grep -c '^"lib' "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" || true)
[ "$n" -gt 0 ] || die "loaders.cache lists no loaders"
note "loaders.cache: $n loaders"

# GSettings schemas: only GTK's own (no demo/example schemas).
mkdir -p "$DEST/share/glib-2.0/schemas"
for f in "$MINGW"/share/glib-2.0/schemas/org.gtk.Settings.*.gschema.xml; do
	cp_mingw "${f#$MINGW/}" "${f#$MINGW/}"
done
"$MINGW/bin/glib-compile-schemas.exe" "$DEST/share/glib-2.0/schemas"

# Icon themes. Adwaita 4x/5x inherits AdwaitaLegacy (old stock-style icons)
# and hicolor. share\icons is also Sylpheed's THEMEDIR; Sylpheed only looks
# for flat <name>.png overrides there, so the theme subdirectories do not
# interfere.
for t in Adwaita AdwaitaLegacy hicolor; do
	cp_mingw share/icons/$t share/icons/$t
	"$MINGW/bin/gtk-update-icon-cache-3.0.exe" -q -f -t "$DEST/share/icons/$t"
done

# Key themes selectable through gtk-key-theme-name (tiny CSS files).
for t in Default Emacs; do
	cp_mingw share/themes/$t/gtk-3.0 share/themes/$t/gtk-3.0
done

# iso-codes for gtkspell3's language names.
for f in iso_639.xml iso_3166.xml; do
	cp_mingw share/xml/iso-codes/$f share/xml/iso-codes/$f
done

# Library message catalogs for the languages Sylpheed ships.
# (gtk30-properties only translates property descriptions for GTK Inspector
# and UI designers; left out, it is 7 MB.)
for d in "$PREFIX"/share/locale/*/; do
	lang=$(basename "$d")
	for dom in gtk30 glib20 gdk-pixbuf gtkspell3 \
		   iso_639 iso_3166 at-spi2-core; do
		f=share/locale/$lang/LC_MESSAGES/$dom.mo
		[ -f "$MINGW/$f" ] && cp_mingw "$f" "$f"
	done
done

# fontconfig reads <prefix>\etc\fonts when it is used (pangoft2); ship its
# configuration whenever its DLL got bundled.
if [ -e "$DEST/libfontconfig-1.dll" ]; then
	cp_mingw etc/fonts etc/fonts
fi

# GTK settings for Windows (see BUILD-WIN64.md, "Portable folder").
mkdir -p "$DEST/etc/gtk-3.0"
cat > "$DEST/etc/gtk-3.0/settings.ini" <<'EOF'
# GTK 3 defaults for the Windows build of Sylpheed.
# Per-user overrides: %LOCALAPPDATA%\gtk-3.0\settings.ini
[Settings]
# Adwaita (built into GTK) rather than the unmaintained built-in "win32"
# theme. Light variant: GTK 3 does not follow the Windows dark mode setting.
gtk-theme-name=Adwaita
gtk-icon-theme-name=Adwaita
# No gtk-font-name: GDK takes the Windows message font (normally Segoe UI 9)
# and the DPI from the system, so scaling follows Windows' display settings.
EOF

# ---------------------------------------------------------------------------
# CA certificates for TLS.
#
# libsylph/ssl.c (ssl_init) looks, on Windows, for <configdir>\certs\ and
# <configdir>\{ca-certificates,ca-bundle,ca-root,certs}.crt / cert.pem, then
# for the directory <exe>\etc\ssl\certs\ and the same file names in it, then
# in <exe>\etc\ssl and <exe>\etc. syl-auth-helper.exe (OAuth2 token request)
# only uses <exe>\etc\ssl\certs\certs.crt and switches peer verification OFF
# if that file is missing. So the bundle is installed under exactly that name.
cp_mingw etc/ssl/certs/ca-bundle.crt etc/ssl/certs/certs.crt
# curl.exe (update check) has /ucrt64/etc/ssl/certs/ca-bundle.crt compiled in,
# which does not exist outside MSYS2; on Windows it also looks for
# curl-ca-bundle.crt next to curl.exe.
cp "$DEST/etc/ssl/certs/certs.crt" "$DEST/curl-ca-bundle.crt"

# ---------------------------------------------------------------------------
# Licences and package versions of everything taken from MSYS2

sort -u "$MINGW_FILES" -o "$MINGW_FILES"
# (files not owned by any package, such as the hand-built compface, make
# pacman fail; that is expected)
PKGS=$( (tr '\n' '\0' < "$MINGW_FILES" | xargs -0 pacman -Qqo 2>/dev/null || true) | sort -u)
mkdir -p "$DEST/doc/licenses"
NOLIC=
for p in $PKGS; do
	base=${p#${MINGW_PACKAGE_PREFIX:-mingw-w64-ucrt-x86_64}-}
	if [ -d "$MINGW/share/licenses/$base" ]; then
		cp -r "$MINGW/share/licenses/$base" "$DEST/doc/licenses/$base"
	else
		NOLIC="$NOLIC $base"
	fi
done
# compface is not a pacman package. Its copyright and licence are in the
# README of the source archive; pass COMPFACE_SRC=<unpacked source dir> to
# include it.
if [ -n "$COMPFACE_SRC" ] && [ -f "$COMPFACE_SRC/README" ]; then
	mkdir -p "$DEST/doc/licenses/compface"
	cp "$COMPFACE_SRC/README" "$DEST/doc/licenses/compface/README"
else
	NOLIC="$NOLIC compface"
fi
{
	echo "Third-party components bundled in this folder"
	echo "(MSYS2 package, version, licence as declared by the package):"
	echo
	for p in $PKGS; do
		LC_ALL=C pacman -Qi "$p" | awk -v p="$p" '
			/^Version/  { v = substr($0, index($0, ":") + 2) }
			/^Licenses/ { l = substr($0, index($0, ":") + 2) }
			END { printf "%-48s %-22s %s\n", p, v, l }'
	done
	echo
	echo "Not from pacman: libcompface-1.dll (compface 1.5.2, built by hand,"
	echo "see BUILD-WIN64.md section 4)."
	echo
	echo "Licence texts are in doc\\licenses\\<package>\\."
	[ -z "$NOLIC" ] || echo "MSYS2 provides no licence text for:$NOLIC"
} | tr -d '\r' | sed 's/$/\r/' > "$DEST/doc/PACKAGES.txt"
[ -z "$NOLIC" ] || note "no licence text found for:$NOLIC"

# ---------------------------------------------------------------------------
# Optional test launcher (TEST_LAUNCHER=1): runs the folder with a minimal
# PATH, a profile inside the folder and its own instance id and IPC port, so
# that it never talks to an installed Sylpheed or its profile. The installer
# script excludes run-test.bat and profile\.

if [ -n "$TEST_LAUNCHER" ]; then
	sed 's/$/\r/' > "$DEST/run-test.bat" <<'EOF'
@echo off
rem Test launcher for this portable Sylpheed folder.
rem Uses only Windows' own directories on PATH (nothing from MSYS2), the
rem profile folder next to this file, IPC port 50297 and a separate instance
rem id, so it does not interfere with an installed Sylpheed.
rem Extra arguments are passed on, e.g. "run-test.bat --debug" or
rem "run-test.bat --exit".
setlocal
set "HERE=%~dp0"
set "PATH=%SystemRoot%\System32;%SystemRoot%"
"%HERE%sylpheed.exe" --configdir "%HERE%profile" --ipcport 50297 --instance-id SylpheedGtk3Portable %*
EOF
fi

note "files:$(find "$DEST" -type f | wc -l), size: $(du -sh "$DEST" | cut -f1)"
note "done."
