;!define HAVE_AUTOENC_PLUGIN

!define PRODUCT_NAME "Zylpheed"
; override with makensis -DPRODUCT_VERSION=...
!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION "0.1.0"
!endif

!define PRODUCT_PUBLISHER "Creath and Claude"
!define PRODUCT_WEB_SITE "https://github.com/creathcarter/zylpheed"

!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\zylpheed.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define MEMENTO_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MEMENTO_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"

!define INST_NAME ${PRODUCT_NAME}
!define INST_VERSION ${PRODUCT_VERSION}
!define INST_FILENAME ${PRODUCT_NAME}

; the upstream project this fork is based on
!define ORIG_WEB_SITE "http://sylpheed.sraoss.jp/"
