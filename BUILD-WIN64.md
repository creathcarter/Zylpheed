# Building Sylpheed for 64-bit Windows (MSYS2 UCRT64)

This describes how to reproduce the 64-bit Windows build of this tree from a clean
Windows 10/11 machine. The current tree builds against **GTK+ 2.24** (phases 0–3 of
`PORTING_PLAN.md`); the switch to GTK 3.24 comes later.

Everything below was done on Windows 11 (x64) in November 2026. Package versions are
the ones that were current in MSYS2 at that time; newer versions will usually work,
but see "Known toolchain issues" if they do not.

## 1. Install MSYS2

`winget` is not available on every Windows edition (it is missing on LTSC), so the
self-extracting base archive is used. It needs no administrator rights.

1. Download `msys2-base-x86_64-<date>.sfx.exe` from <https://repo.msys2.org/distrib/x86_64/>
   (the build used `msys2-base-x86_64-20261120.sfx.exe`) and check its SHA-256 against
   the digest published on the matching GitHub release
   (<https://github.com/msys2/msys2-installer/releases>).
2. Extract it so that the tree ends up in `C:\msys64`:

   ```
   msys2-base-x86_64-20261120.sfx.exe -y -oC:\
   ```

3. Run the first login once (creates the keyring and home directory), then update:

   ```
   C:\msys64\usr\bin\bash.exe -lc "true"
   C:\msys64\usr\bin\bash.exe -lc "pacman -Syuu --noconfirm"
   ```

   Run the update command again until it reports nothing to do. Messages such as
   `keyserver refresh failed` during the first login are harmless.

   If many mirrors are unreachable from your network, pacman falls back to the next
   mirror; `repo.msys2.org` itself is sufficient.

## 2. The shell environment

All build commands run in the **UCRT64** environment. From an MSYS2 UCRT64 terminal
(`C:\msys64\ucrt64.exe`) just type them. From another shell (cmd, PowerShell, Git Bash),
use:

```
set MSYSTEM=UCRT64
set CHERE_INVOKING=1
C:\msys64\usr\bin\bash.exe -lc "<command>"
```

When calling MSYS2's bash from Git Bash, clear Git Bash's environment first, because
both are MSYS runtimes and their variables conflict:

```
env -i SYSTEMROOT="$SYSTEMROOT" WINDIR="$WINDIR" TEMP="$TEMP" TMP="$TMP" \
    MSYSTEM=UCRT64 CHERE_INVOKING=1 MSYS2_PATH_TYPE=minimal \
    /c/msys64/usr/bin/bash.exe -lc "<command>"
```

## 3. Packages

```
P=mingw-w64-ucrt-x86_64
pacman -S --needed --noconfirm \
    make autoconf automake libtool flex bison gettext gettext-devel patch unzip \
    $P-toolchain $P-pkgconf \
    $P-gtk2 $P-gtk3 $P-glib2 \
    $P-openssl $P-gpgme $P-oniguruma $P-curl $P-libiconv \
    $P-gtkspell3 $P-enchant $P-gettext-tools $P-gettext-runtime
```

`gtk3`, `gtkspell3` and `enchant` are not used by the current (GTK 2) build; they are
installed now for phase 4.

Versions used for the reference build:

| Package | Version |
|---|---|
| gcc (mingw-w64-ucrt-x86_64-gcc) | 16.2.0-7 |
| binutils | 2.47-5 |
| mingw-w64 crt / headers | 14.0.0.r598 |
| make | 4.4.1-3 |
| autoconf (wrapper; runs autoconf 2.73) | 20260320-3 |
| automake (wrapper; runs automake 1.18.1) | 20260320-3 |
| libtool | 2.6.2-2 |
| flex / bison | 2.6.4-5 / 3.8.2-5 |
| gettext (MSYS) / gettext-tools (UCRT64) | 0.22.5-1 / 1.0-1 |
| pkgconf | 3.0.11-2 |
| **gtk2** | **2.24.33-8** |
| gtk3 | 3.24.52-1 |
| glib2 | 2.90.1-1 |
| pango / cairo / gdk-pixbuf2 / atk | 1.58.2 / 1.18.6 / 2.44.8 / 2.62.1 |
| openssl | 4.1.0-1 |
| gpgme | 2.2.0-1 |
| oniguruma | 6.9.10-1 |
| curl | 8.23.0-3 |
| libiconv | 1.19.1-1 |
| gtkspell3 / enchant | 3.0.10-4 / 2.8.21-3 |

Not packaged in MSYS2:

- **GtkSpell 2** (`gtkspell-2.0`). Only `gtkspell3` exists. The GTK 2 build is therefore
  configured without spell checking (configure reports `GtkSpell : no`). Building
  gtkspell 2.0.16 would require porting it to the enchant-2 API; spell checking returns
  with gtkspell3 in phase 4.
- **compface** (X-Face support). Build it from source as described next.

## 4. compface (X-Face)

compface 1.5.2 is five C files; no configure run is needed. The reference build used
the Debian source archive `libcompface_1.5.2.orig.tar.gz` (SHA-256
`501c28708c15f6edd1697b8be1f08ae4490d8b35216176525342b2e0621b5982`, as listed in
`libcompface_1.5.2-6.1.dsc`) from <https://deb.debian.org/debian/pool/main/libc/libcompface/>.

In a UCRT64 shell:

```
mkdir compface-src compface-build
tar -C compface-src -xzf libcompface_1.5.2.orig.tar.gz
SRC=$PWD/compface-src/libcompface-1.5.2.orig
cd compface-build
for f in arith file compress gen uncompface; do
    gcc -std=gnu89 -O2 -DHAVE_CONFIG_H -DSTDC_HEADERS=1 -DHAVE_ERRNO_H=1 \
        -DHAVE_FCNTL_H=1 -DHAVE_STRERROR=1 -I$SRC -c $SRC/$f.c -o $f.o
done
gcc -shared -o libcompface-1.dll arith.o file.o compress.o gen.o uncompface.o \
    -Wl,--out-implib,libcompface.dll.a
install -m 644 $SRC/compface.h /ucrt64/include/compface.h
install -m 755 libcompface-1.dll /ucrt64/bin/libcompface-1.dll
install -m 644 libcompface.dll.a /ucrt64/lib/libcompface.dll.a
```

Build it as a DLL with an import library, **not** as a static `libcompface.a`.
`configure` adds `-lcompface` to the global `LIBS`, so libsylph and the plug-in library
link against it too; when only a static archive exists, libtool silently builds
`libsylph` and `libsylpheed-plugin` as static libraries, no DLLs are produced and plug-ins
cannot be built or loaded. (Check: the build log must not contain "cannot link to static
lib archive", and `build-win64/src/.libs/` must contain `libsylpheed-plugin-0-1.dll`.)

`libcompface-1.dll` must be shipped next to `sylpheed.exe`. These three files are not
owned by pacman; remove them by hand if you want to uninstall.

## 5. Configure and build

From the top of the source tree, in a UCRT64 shell:

```
NOCONFIGURE=1 ./autogen.sh
mkdir -p build-win64 && cd build-win64
../configure --prefix=$PWD/dist \
    --with-localedir=share/locale --with-themedir=share/icons \
    --enable-oniguruma --enable-threads
make -j8
make install
```

The options match the official Windows build in `makewin32.sh` (Oniguruma, threads,
locale and theme directories relative to the installation). GPGME, OpenSSL, libcurl,
IPv6 and compface are enabled by default when found. The configure summary should read:

```
GnuPG         : yes
JPilot        : no
LDAP          : no
OpenSSL       : yes
iconv         : yes
compface      : yes
IPv6          : yes
GtkSpell      : no
Oniguruma     : yes
GThread       : yes
libcurl       : yes
```

Notes:

- `autogen.sh` uses `aclocal`/`automake` from `PATH`; set `ACLOCAL`/`AUTOMAKE` to use
  specific versions. `NOCONFIGURE=1` stops it from running configure in the source
  directory. Running it rewrites the tracked `ltmain.sh` and `missing` with the
  installed libtool/automake versions; that is expected.
- The build is out of tree (`build-win64/`). The source tree only receives the files
  autotools generate (`configure`, `Makefile.in`, ...) and the `po/*.gmo` catalogues.
- The project uses `AM_MAINTAINER_MODE`, so editing a `Makefile.am` or `configure.ac`
  does not regenerate anything automatically. Rerun `NOCONFIGURE=1 ./autogen.sh` and
  then `./config.status` (or configure) in the build directory.
- The plug-ins are built but not installed by `make install`. To install one, run
  `make install-plugin` in `build-win64/plugin/attachment_tool` (or `plugin/test`).
  On Windows, Sylpheed loads plug-ins from `<configdir>\plugins` and from
  `plugins\` next to `sylpheed.exe`, not from `lib\sylpheed\plugins`; copy the DLL to
  one of those to test it.
- `configure` requires GTK+ 2.24 and compiles with `-DGTK_DISABLE_SINGLE_INCLUDES
  -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES`, so only `<gtk/gtk.h>`, `<gdk/gdk.h>`,
  `<gdk/gdkkeysyms.h>` and the backend headers (`<gdk/gdkwin32.h>`, `<gdk/gdkx.h>`) may
  be included.
- `makewin32.sh` passed `-mthreads -mtune=core2 -static-libgcc`; these are not needed
  with the UCRT64 toolchain and were not used.

The result is `build-win64/dist/bin/sylpheed.exe` (PE32+ x86-64 GUI binary) plus
`libsylph-0-1.dll`, `libsylpheed-plugin-0-1.dll` and `syl-auth-helper.exe`.

## 6. Running the build

The DLLs from MSYS2 must be found at run time. For development, put
`C:\msys64\ucrt64\bin` first on `PATH` (or start from a UCRT64 shell). Bundling the
runtime DLLs next to the executable is installer work (phase 5).

**Always test with a throwaway profile.** Sylpheed's single-instance check on Windows
uses a named mutex (`Sylpheed`) and TCP port 50215 on localhost. If another Sylpheed is
running, a new instance with default settings hands its command line to the running
one over that port and exits. Use a separate instance id, port and configuration
directory:

```
sylpheed.exe --configdir C:\path\to\scratch\profile --ipcport 50299 --instance-id SylpheedTest
```

`--configdir` replaces the default `%APPDATA%\Sylpheed`. A `sylpheed.ini` next to
`sylpheed.exe` can set `configdir` and `ipcport` as well (section `[Sylpheed]`).

On a fresh profile the main window opens with the "Mailbox setting" dialog, followed by
"New account setup".

## 7. Compiler warnings used for the Win64 audit

The normal build uses `-g -O2 -std=gnu17`. To repeat the audit build with the warnings
for pointer/integer casts and format strings, override `CFLAGS` at make time:

```
make clean
make -k CFLAGS="-g -O2 -std=gnu17 -mms-bitfields -mwindows -Wall -Wextra \
  -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers \
  -Wno-deprecated-declarations -Wpointer-to-int-cast -Wint-to-pointer-cast \
  -Wformat=2 -Wno-format-nonliteral -Wincompatible-pointer-types -Wint-conversion \
  -Wcast-function-type \
  -DGNULIB_overrides_printf -DGNULIB_overrides_fprintf -DGNULIB_overrides_sprintf \
  -DGNULIB_overrides_snprintf -DGNULIB_overrides_vprintf -DGNULIB_overrides_vfprintf \
  -DGNULIB_overrides_vsprintf -DGNULIB_overrides_vsnprintf \
  -DGNULIB_overrides_asprintf -DGNULIB_overrides_vasprintf"
```

The `GNULIB_overrides_*` defines are for the audit build only. MSYS2's `libintl.h`
replaces `printf`, `fprintf`, `sprintf` and friends with `libintl_*` functions that have
no format attribute, which hides those calls from `-Wformat`. The defines switch that
redirection off so the compiler can check them. Do not use them for release builds:
the `libintl_*` versions support positional arguments (`%1$s`) in translated strings,
which the Windows C runtime does not.

To check progress on sealed struct access (phase 1 of `PORTING_PLAN.md`), build with
`-DGSEAL_ENABLE` added to the normal flags (`make -k CFLAGS="-g -O2 -std=gnu17
-mms-bitfields -mwindows -DGSEAL_ENABLE"`). The remaining errors are listed in the
phase 1 report; they belong to code that phases 2–4 replace. `GTK_DISABLE_DEPRECATED`
and `GDK_DISABLE_DEPRECATED` cannot be enabled until the phase 2 widget replacements are
done.

## 8. Known toolchain issues and how the tree handles them

- **GCC 15+ defaults to C23.** GTK 2 headers declare callbacks with empty parameter
  lists (for example `GtkItemFactoryCallback`), which mean `(void)` in C23 and turn
  hundreds of callback tables into errors. `configure.ac` adds `-std=gnu17` for GCC
  unless `CFLAGS` already contains a `-std=` option.
- **GCC 14+ makes implicit function declarations and incompatible pointer types
  errors.** The missing prototypes this exposed have been fixed in the source.
- **Newer mingw-w64 `shlobj.h`** declares `Folder`, `FolderItem`, `SORT_ASCENDING` and
  `SORT_DESCENDING`, which clash with libsylph; `src/filesel.c` renames them around that
  include.
- **compface.h** defines `WORD` and `COMP` as macros, which break `<windows.h>`;
  `src/headerview.c` undefines them after the include.
- **OpenSSL 4.x** removed `ASN1_STRING_data`; the code uses `ASN1_STRING_get0_data` and
  the OpenSSL 1.1+ initialisation API, with fallbacks for older OpenSSL.
- The MSYS2 GLib (2.90) and GTK 2 headers produce many `-Wdeprecated-declarations`
  warnings (for example `GTimeVal`, `GTypeDebugFlags`); they are expected.
