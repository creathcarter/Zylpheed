# Building Sylpheed for 64-bit Windows (MSYS2 UCRT64)

This describes how to reproduce the 64-bit Windows build of this tree from a clean
Windows 10/11 machine. The current tree builds against **GTK+ 3.24** (phase 4 of
`PORTING_PLAN.md`). The last GTK+ 2.24 state (end of phase 3) is not kept in the tree;
see section 9.

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
    $P-gtkspell3 $P-enchant $P-iso-codes $P-gettext-tools $P-gettext-runtime
```

For the installer (section 10) also install NSIS:

```
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-nsis
```

`gtk2` is only needed to build the GTK+ 2 reference state (section 9). `iso-codes` is
used by gtkspell3 for language names; without it gtkspell prints warnings about
`iso_639.xml` / `iso_3166.xml`. Spell checking uses enchant-2; on Windows its
`winspell` provider uses the dictionaries of Windows itself, so no extra dictionary
package is needed.

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
| gtk2 (reference build only) | 2.24.33-8 |
| **gtk3** | **3.24.52-1** |
| adwaita-icon-theme / hicolor-icon-theme | 51.0-1 / 0.18-1 |
| iso-codes | 4.20.1-1 |
| glib2 | 2.90.1-1 |
| pango / cairo / gdk-pixbuf2 / atk | 1.58.2 / 1.18.6 / 2.44.8 / 2.62.1 |
| openssl | 4.1.0-1 |
| gpgme | 2.2.0-1 |
| oniguruma | 6.9.10-1 |
| curl | 8.23.0-3 |
| libiconv | 1.19.1-1 |
| gtkspell3 / enchant | 3.0.10-4 / 2.8.21-3 |
| ca-certificates (bundled CA list) | 20260816-6 |
| nsis (installer only) | 3.13-2 |

Not packaged in MSYS2:

- **GtkSpell 2** (`gtkspell-2.0`). Only `gtkspell3` exists, which the GTK 3 build uses
  (`configure` looks for `gtkspell3-3.0` and `enchant-2`). The GTK 2 reference build is
  configured without spell checking.
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
lib archive", and `build-win64/src/.libs/` must contain `libsylpheed-plugin-0-6.dll`.)

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
GtkSpell      : yes
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
- `configure` requires GTK+ 3.24 (`pkg-config gtk+-3.0`) and compiles with
  `-DGTK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES`, so only
  `<gtk/gtk.h>`, `<gdk/gdk.h>`, `<gdk/gdkkeysyms.h>` and the backend headers
  (`<gdk/gdkwin32.h>`, `<gdk/gdkx.h>`) may be included.
- The build produces about 1800 `-Wdeprecated-declarations` warnings: `GtkTable`,
  `GtkHBox`/`GtkVBox`, stock items, `GtkMisc`, `GtkUIManager`, `GtkStatusIcon`,
  `gtk_menu_popup` and similar are deprecated in GTK 3 but still work, and stay until
  phase 6. They are warnings, not errors. `--disable-deprecated` adds
  `-DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED` and therefore does not build yet.
- MSYS2's `gtk+-3.0.pc` lists `-luuid`, which exists only as a static library. libtool
  would then build `libsylpheed-plugin` and the plug-ins as static libraries;
  `configure` therefore passes it as `-Wl,-luuid` (check: `build-win64/src/.libs/`
  must contain `libsylpheed-plugin-0-6.dll`).
- The libraries are now `libsylph-0-6.dll` and `libsylpheed-plugin-0-6.dll` (libtool
  version 6:0:0; the GTK 2 builds produced `-0-1`). `make install` does not delete the
  old `-0-1` DLLs from an existing `dist/bin`.
- `makewin32.sh` passed `-mthreads -mtune=core2 -static-libgcc`; these are not needed
  with the UCRT64 toolchain and were not used.

The result is `build-win64/dist/bin/sylpheed.exe` (PE32+ x86-64 GUI binary) plus
`libsylph-0-6.dll`, `libsylpheed-plugin-0-6.dll` and `syl-auth-helper.exe`.

## 6. Running the build

The DLLs from MSYS2 must be found at run time. For development, put
`C:\msys64\ucrt64\bin` first on `PATH` (or start from a UCRT64 shell). GTK 3 then also
finds its data (Adwaita icons, GSettings schemas, gdk-pixbuf loaders) under
`C:\msys64\ucrt64\share` and `lib`. To run without MSYS2, make the portable folder
described in section 10.

Appearance: GTK 3 has no gtkrc files. Sylpheed installs its own style sheet and, if it
exists, loads `gtk.css` from the configuration directory (in place of the old `gtkrc`
there) with user priority. GTK itself also reads `%LOCALAPPDATA%\gtk-3.0\gtk.css` and
`settings.ini`. `menurc` (accelerators) is loaded and saved as before.

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

Sealed struct access and the GTK 2 deprecations were cleared before the switch: the last
GTK 2 state built with `-DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED`, and GTK 3
seals all structures. To list the remaining GTK 3 deprecations, count the
`-Wdeprecated-declarations` warnings of a clean build.

## 8. Known toolchain issues and how the tree handles them

- **GCC 15+ defaults to C23.** The code (and the GTK 2 headers) declare callbacks with
  empty parameter lists, which mean `(void)` in C23 and turn the menu callback tables
  into errors. `configure.ac` adds `-std=gnu17` for GCC unless `CFLAGS` already contains
  a `-std=` option.
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
- `src/libsylpheed-plugin-0.def` lists the exported plug-in API. A function added to
  `src/plugin.c` must be added there too, or plug-ins fail to link.

## 9. The GTK+ 2 reference state

The tree no longer builds against GTK 2. The last GTK 2 state (end of phase 3, plus
two later bug fixes that apply to it) was built once as a snapshot for comparison
testing, outside the repository: `C:\_projects\code\sylpheed\snapshots\gtk2-final\`,
started with `run-test.bat` there (throwaway profile in its `profile` folder, IPC port
50298). It needs the MSYS2 `gtk2` package on `PATH` (`C:\msys64\ucrt64\bin`).

## 10. Portable folder and installer

### Portable folder

`nsis/make-dist-win64.sh` assembles a self-contained folder that runs from any location
without MSYS2. Run it in a UCRT64 shell after `make` and `make install`:

```
nsis/make-dist-win64.sh build-win64 /c/path/to/Sylpheed
```

(Default output: `build-win64/Sylpheed`. The folder is deleted and recreated.) Optional
environment variables: `TEST_LAUNCHER=1` writes `run-test.bat` (see below);
`COMPFACE_SRC=<unpacked compface source>` copies compface's licence notice.

What it puts in the folder:

- `sylpheed.exe`, `syl-auth-helper.exe` and every DLL they need. The DLLs are found by
  walking the PE import tables (`objdump -p`) of every `.exe`/`.dll` in the folder,
  including the run-time modules below, until nothing new turns up. A DLL that is in
  neither `dist/bin`, `/ucrt64/bin` nor `System32` stops the script. The stale GTK 2
  `-0-1` libraries in `dist/bin` are never picked up, because nothing imports them.
- `plugins\attachment_tool.dll` and `plugins\test.dll` from the build tree (`make
  install` does not install plug-ins; on Windows they load from `plugins\` beside the
  exe, see `plugin_init()` in `src/main.c`), plus their READMEs in `doc\plugins`.
- `gspawn-win64-helper.exe` / `-console.exe` (GLib spawning), `gpgme-w32spawn.exe`, and
  `curl.exe` (the update check runs `curl`).
- `lib\gdk-pixbuf-2.0\2.10.0\loaders\` and a `loaders.cache` regenerated inside the
  folder (relative paths; the script fails if an MSYS2 path ends up in it). The SVG
  loader (librsvg) is needed: current Adwaita icons are SVG.
- `lib\enchant-2\` with the `winspell` (Windows spell checker, no dictionaries to ship)
  and `hunspell` providers; aspell, nuspell (pulls in ICU, ~40 MB) and voikko are left
  out. Hunspell dictionaries can be added to `share\hunspell`.
- `share\glib-2.0\schemas\gschemas.compiled` compiled from GTK's own schemas
  (`org.gtk.Settings.*`; used by the file chooser).
- `share\icons\Adwaita`, `AdwaitaLegacy` (old stock-style icons; Adwaita inherits from
  it) and `hicolor`, with icon caches regenerated. `share\icons` is also Sylpheed's
  `THEMEDIR`; Sylpheed only looks for flat `<name>.png` overrides there.
- `share\themes\{Default,Emacs}\gtk-3.0` (key themes), `share\xml\iso-codes\iso_639.xml`
  and `iso_3166.xml` (language names in the gtkspell menu), `share\enchant-2`.
- `share\locale`: Sylpheed's catalogs and, for the same languages, those of GTK, GLib,
  gdk-pixbuf, gtkspell3, iso-codes and at-spi2-core (`gtk30-properties` is left out: it
  only translates property descriptions for GTK Inspector, 7 MB).
- `share\sylpheed` (manual, FAQ), `etc\fonts` (fontconfig is linked in through
  pango/cairo), `etc\gtk-3.0\settings.ini` (below).
- `etc\ssl\certs\certs.crt` and `curl-ca-bundle.crt` (CA certificates, below).
- `doc\`: Sylpheed's `COPYING`, `COPYING.LIB`, `LICENSE`, `README`, `NEWS`;
  `doc\licenses\<package>\` from MSYS2's `share/licenses` for every bundled package
  (found with `pacman -Qo`), and `doc\PACKAGES.txt` with package versions and declared
  licences. MSYS2 ships no licence text for atk, ca-certificates, libidn2 and oniguruma;
  `PACKAGES.txt` says so.

Layout: everything sits next to `sylpheed.exe`, as in the official Windows builds. GTK,
GLib, gdk-pixbuf and fontconfig find their data relative to their DLL directory (a
trailing `bin` is stripped if present), and Sylpheed relative to the exe
(`LOCALEDIR=share/locale`, `THEMEDIR=share/icons`).

enchant is the exception: it only relocates when its DLL is in a `bin` directory and
otherwise looks in `/ucrt64/lib/enchant-2`, so spell checking found no dictionaries.
`src/main.c` therefore calls `enchant_set_prefix_dir(<exe dir>)` on Windows when
`<exe dir>\lib\enchant-2` exists (so running `dist/bin/sylpheed.exe` against MSYS2 is
unaffected).

Size with the reference package set: 104 MB, about 3250 files, 69 DLLs.

**CA certificates.** `ssl_init()` (`libsylph/ssl.c`) does not use the Windows
certificate store. On Windows it looks for a `certs` directory in the configuration
directory, else for the directory `<exe>\etc\ssl\certs\`; then for one of
`ca-certificates.crt`, `ca-bundle.crt`, `ca-root.crt`, `certs.crt`, `cert.pem` in the
configuration directory, in that certs directory, in `<exe>\etc\ssl` and in `<exe>\etc`.
`syl-auth-helper.exe` (OAuth2 token request, libcurl) only uses
`<exe>\etc\ssl\certs\certs.crt` and turns peer verification **off** if that file is
missing. The script therefore installs MSYS2's `ca-bundle.crt` (package
ca-certificates, Mozilla's list) as `etc\ssl\certs\certs.crt`. `curl.exe` (update
check) looks for `<exe>\..\etc\ssl\certs\ca-bundle.crt` (outside the folder) and then
for `curl-ca-bundle.crt` next to itself, so the same file is also copied there. The list
is a snapshot: rebuild the folder to update it. A user can override it with
`ca-certificates.crt` (etc.) in the configuration directory; `trust.crt` there still
holds the certificates accepted by the user.

**GTK settings.** `etc\gtk-3.0\settings.ini` (GKeyFile syntax, `#` comments) sets
`gtk-theme-name=Adwaita` and `gtk-icon-theme-name=Adwaita`, so the light Adwaita theme
is used instead of GTK's built-in, little-maintained `win32` theme. No dark variant is
forced (GTK 3 does not follow the Windows dark mode setting; a user can set
`gtk-application-prefer-dark-theme=1` in `%LOCALAPPDATA%\gtk-3.0\settings.ini`). No
font is set: GDK takes the Windows message font (normally Segoe UI 9 pt) and the system
DPI, so text follows the Windows display settings.

**Test launcher.** With `TEST_LAUNCHER=1`, `run-test.bat` in the folder starts
`sylpheed.exe` with `PATH` reduced to `%SystemRoot%\System32;%SystemRoot%`,
`--configdir <folder>\profile`, `--ipcport 50297` and `--instance-id
SylpheedGtk3Portable`; extra arguments (`--debug`, `--exit`) are passed on. It runs in
the foreground of its console window, which then also shows the `--debug` output. The
current snapshot is `C:\_projects\code\sylpheed\snapshots\gtk3-portable\`.

Checked with `PATH` set to `C:\Windows\System32;C:\Windows` only and throwaway profiles:
the first-run "Mailbox setting" dialog appears; with a prepared profile the main and
compose windows open, both plug-ins load, spell checking finds the Windows dictionaries
(en_CA, en_LR, en_PH, en_US on the test machine), `ssl_init()` finds `certs.crt`, the
update check starts curl, no module outside the folder or `C:\Windows` is loaded, and
GTK prints no warnings (no missing icons or pixbuf loaders).

### Installer

NSIS 3.13 from MSYS2 (section 3) builds only native 64-bit installers
(`amd64-unicode`). `nsis/make-installer-win64.sh` generates the uninstaller's file
list from the portable folder and runs `makensis` on `nsis/sylpheed.nsi`:

```
nsis/make-installer-win64.sh /c/path/to/Sylpheed /c/path/to/output
```

(or `nsis\compile.bat <folder> [<output>]` from cmd.exe). The result is
`Sylpheed-<version>-x64_setup.exe` (about 28 MB). It installs to
`%ProgramFiles%\Sylpheed`, uses the 64-bit registry view, keeps the start menu and
desktop shortcuts, the `mailto:`/`.eml` registration and the language selection
(English, Spanish, Japanese), and needs no third-party NSIS plug-ins. See
`nsis/README.txt` for options and for what was dropped from the official 32-bit
installer.

Do not test the installer on a machine with a Sylpheed installation and profile you
care about: the installed program uses `%APPDATA%\Sylpheed` like the official one, and
the `HKLM\Software\Classes` association keys are shared with a 32-bit installation.
