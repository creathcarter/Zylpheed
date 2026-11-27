NSIS scripts for Zylpheed installer (64-bit Windows, GTK 3)
===========================================================

The installer packages a self-contained Zylpheed folder (program, all DLLs,
GTK 3 run-time data, plug-ins). Both steps run in MSYS2 (UCRT64); see
BUILD-WIN64.md in the top directory for the build itself.

1. Build and install Zylpheed (BUILD-WIN64.md, section 5):

     cd build-win64 && make && make install

2. Make the portable folder:

     nsis/make-dist-win64.sh build-win64 <dist folder>

   The folder runs as it is, from any location, without MSYS2 on PATH.
   With TEST_LAUNCHER=1 the script also writes run-test.bat (throwaway
   profile in <dist folder>\profile, own IPC port and instance id).
   COMPFACE_SRC=<compface source dir> adds the compface licence text.

3. Install NSIS:

     pacman -S mingw-w64-ucrt-x86_64-nsis

   No third-party NSIS plug-ins are needed for zylpheed.nsi.

4. Build the installer:

     nsis/make-installer-win64.sh <dist folder> [<output folder>] [-D...]

   or, from cmd.exe, "compile.bat <dist folder> [<output folder>]".
   The result is Zylpheed-<version>-x64_setup.exe. The script generates the
   uninstaller's file list from the dist folder, so the uninstaller removes
   exactly the files it installed.

   Options (makensis -D...): PRODUCT_VERSION=x.y.z, WITH_TEST_PLUGIN (offer
   the "test" demo plug-in; it is left out by default because it adds demo
   menus and redirects the update check URL), TARGET_X86 (when using an x86
   NSIS; untested).

Differences from the official 32-bit installer:

- Product name Zylpheed: installs to Program Files\Zylpheed, uninstall entry
  "Zylpheed <version>", start menu folder Zylpheed, App Paths zylpheed.exe,
  ProgIDs Zylpheed.EML and Zylpheed.Url.MailTo, Clients\Mail\Zylpheed.
  Nothing of an installed Sylpheed (keys, files, shortcuts, profile) is read,
  changed or removed; both can be installed side by side.
- 64-bit only: Program Files (not "Program Files (x86)"), 64-bit registry
  view.
- Files come from the dist folder recursively instead of per-DLL lists.
- "Zylpheed is running" is detected through Zylpheed's own single-instance
  mutex "Zylpheed" (System plug-in) instead of the nsProcess plug-in; the
  uninstaller asks the user to quit Zylpheed instead of killing it.
- Dropped: Quick Launch shortcut (no Quick Launch in Windows 10/11), writing
  the UserChoice keys for mailto:/.eml (protected since Windows 8), and
  writing the shared keys Software\Classes\mailto, Software\Classes\.eml and
  the HKCU Software\Clients\Mail default (they may belong to another mail
  client); choose Zylpheed in Settings > Apps > Default apps. Also dropped:
  README-win32*.txt.
- Optional, off unless their folders are provided: sylfilter
  (-DHAVE_SYLFILTER, folder sylfilter\), bsfilter (-DHAVE_BSFILTER, folder
  bsfilter\), autoenc (-DHAVE_AUTOENC_PLUGIN).
- plugin-updater.nsi and update-manager.nsi (helpers for the official update
  service) are not built; they need the nsisunz and nsProcess plug-ins.
