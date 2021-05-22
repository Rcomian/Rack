!include "MUI2.nsh"

Name "FreeRack ${VERSION}"
OutFile "installer.exe"
SetCompressor /solid "lzma"
CRCCheck On

; Default installation folder
InstallDir "$PROGRAMFILES\FreeRack\Rack"

; Get installation folder from registry if available
InstallDirRegKey HKLM "Software\FreeRack\Rack" ""

; Request admin permissions so we can install to Program Files and add a registry entry
RequestExecutionLevel admin


; MUI pages

!define MUI_ICON "icon.ico"
!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP "installer-banner.bmp" ; 150x57
;!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp" ; 164x314
;!define MUI_UNWELCOMEFINISHPAGE_BITMAP  "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp" ; 164x314

!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\Rack.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch FreeRack"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"


; Sections

Section "FreeRack" FREE_RACK_SECTION
	SectionIn RO
	SetOutPath "$INSTDIR"

	File /r "dist\Rack\*"

	; Store installation folder
	WriteRegStr HKLM "Software\FreeRack\Rack" "" "$INSTDIR"

	; Write uninstaller info
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "DisplayName" "FreeRack"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "DisplayIcon" '"$INSTDIR\Rack.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "InstallLocation" '"$INSTDIR"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "Publisher" "FreeRack"
	SectionGetSize ${FREE_RACK_SECTION} $0
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "EstimatedSize" $0
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack" "NoRepair" 1

	; Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"

	; Associate file type
	WriteRegStr HKLM "Software\Classes\.vcv" "" "FreeRack.Patch"
	WriteRegStr HKLM "Software\Classes\FreeRack.Patch" "" "FreeRack Patch"
	WriteRegStr HKLM "Software\Classes\FreeRack.Patch\shell\open\command" "" '"$INSTDIR\Rack.exe" "%1"'

	; Create shortcuts
	CreateDirectory "$SMPROGRAMS"
	CreateShortcut "$SMPROGRAMS\FreeRack.lnk" "$INSTDIR\Rack.exe"
SectionEnd


Section "Uninstall"
	Delete "$INSTDIR\Uninstall.exe"
	Delete "$INSTDIR\*"
	RMDir /r "$INSTDIR\res"
	RMDir "$INSTDIR"
	RMDir "$INSTDIR\.."

	Delete "$SMPROGRAMS\FreeRack.lnk"

	DeleteRegKey HKLM "Software\FreeRack\Rack"
	DeleteRegKey /ifempty HKLM "Software\FreeRack"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FreeRack"
	DeleteRegKey HKLM "Software\Classes\.vcv"
	DeleteRegKey HKLM "Software\Classes\FreeRack.Patch"
SectionEnd


; Functions
