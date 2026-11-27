; Single-file portable Zylpheed (64-bit).
;
; Packs the self-contained folder made by nsis/make-dist-win64.sh into one
; .exe. It is not an installer: when started it
;   1. unpacks the program into %TEMP%\Zylpheed-portable-<build stamp>
;      (only the first time; later starts reuse the folder),
;   2. starts zylpheed.exe from there with the profile in a "profiles"
;      folder beside the portable .exe, passing its own arguments on.
; Nothing is written to the registry or to %APPDATA%.
;
; Normally built through nsis/make-portable-win64.sh.
;
; Defines (makensis -D...):
;   DIST_DIR         folder made by make-dist-win64.sh (required)
;   OUTDIR           where to write the .exe (default: this directory)
;   BUILD_STAMP      unique per build; names the unpack folder (required)
;   PRODUCT_VERSION  see zylpheed-defs.nsh

Unicode true
!ifndef TARGET_X86
Target amd64-unicode
!endif

!include "zylpheed-defs.nsh"
!include "FileFunc.nsh"

!ifndef DIST_DIR
!error "DIST_DIR is not defined; build with make-portable-win64.sh"
!endif
!ifndef BUILD_STAMP
!error "BUILD_STAMP is not defined; build with make-portable-win64.sh"
!endif
!ifndef OUTDIR
!define OUTDIR "."
!endif

SetCompressor /SOLID lzma

Name "${PRODUCT_NAME}"
OutFile "${OUTDIR}\${INST_FILENAME}-${INST_VERSION}-x64_portable.exe"
Icon "..\src\icons\zylpheed.ico"
RequestExecutionLevel user
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} (portable)"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 1999-2018 Hiroyuki Yamamoto"
VIAddVersionKey "Comments" "Forked from Sylpheed 3.8.0beta1 by Creath and Claude, 2026"

!define UNPACK_PREFIX "Zylpheed-portable-"

Section
  StrCpy $R0 "$TEMP\${UNPACK_PREFIX}${BUILD_STAMP}"
  IfFileExists "$R0\unpacked.ok" run

  ; Remove what older builds of the portable .exe unpacked. The marker goes
  ; first, so a folder that cannot be removed completely (still running) is
  ; not taken for a complete one later.
  FindFirst $R1 $R2 "$TEMP\${UNPACK_PREFIX}*"
  cleanup_loop:
    StrCmp $R2 "" cleanup_done
    StrCmp $R2 "${UNPACK_PREFIX}${BUILD_STAMP}" cleanup_next
    Delete "$TEMP\$R2\unpacked.ok"
    RMDir /r "$TEMP\$R2"
  cleanup_next:
    FindNext $R1 $R2
    Goto cleanup_loop
  cleanup_done:
  FindClose $R1

  Banner::show /NOUNLOAD "Preparing ${PRODUCT_NAME} for its first start..."
  ClearErrors
  SetOutPath "$R0"
  ; zylpheed.ini is left out: the profile location is given below
  File /r /x "run-test.bat" /x "zylpheed.ini" /x "profile" /x "profiles" "${DIST_DIR}\*.*"
  Banner::destroy
  IfErrors 0 unpacked
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} could not be unpacked to:$\r$\n$R0" /SD IDOK
    Abort
  unpacked:
  FileOpen $R1 "$R0\unpacked.ok" w
  FileClose $R1

  run:
  ${GetParameters} $R3
  SetOutPath "$EXEDIR"
  Exec '"$R0\zylpheed.exe" --configdir "$EXEDIR\profiles" $R3'
SectionEnd
