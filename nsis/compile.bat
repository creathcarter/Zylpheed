@echo off
rem Builds the 64-bit installer from the folder made by make-dist-win64.sh.
rem Usage: compile.bat <dist folder> [output folder] [makensis options]
rem Needs MSYS2 in C:\msys64 with mingw-w64-ucrt-x86_64-nsis installed.
rem See README.txt.

if "%~1"=="" (
  echo usage: compile.bat DIST_FOLDER [OUTPUT_FOLDER] [makensis options]
  exit /b 1
)

setlocal
set MSYSTEM=UCRT64
set CHERE_INVOKING=1
set MSYS2_PATH_TYPE=minimal
cd /d "%~dp0"
C:\msys64\usr\bin\bash.exe -lc "./make-installer-win64.sh '%~1' '%~2' %3 %4 %5 %6"

rem The helper installers of the official 32-bit build (not built here):
rem makensis plugin-updater.nsi   (needs the nsisunz plug-in)
rem makensis update-manager.nsi   (needs the nsProcess plug-in)
