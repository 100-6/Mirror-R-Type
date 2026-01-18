; R-Type Game Installer
; NSIS Script for Windows Installer

;--------------------------------
; Include Modern UI
!include "MUI2.nsh"
!include "x64.nsh"

;--------------------------------
; General

; Name and file
Name "R-Type Game"
OutFile "RType-Setup.exe"
Unicode True

; Default installation folder
InstallDir "$PROGRAMFILES64\R-Type"

; Get installation folder from registry if available
InstallDirRegKey HKCU "Software\R-Type" ""

; Request application privileges for Windows Vista and higher
RequestExecutionLevel admin

;--------------------------------
; Version Information
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "R-Type Game"
VIAddVersionKey "CompanyName" "EPITECH"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2025"
VIAddVersionKey "FileDescription" "R-Type Game Installer"
VIAddVersionKey "FileVersion" "1.0.0.0"

;--------------------------------
; Interface Settings

!define MUI_ABORTWARNING
!define MUI_ICON "installer.ico"
!define MUI_UNICON "installer.ico"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch R-Type"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchGame"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

;--------------------------------
; Installer Sections

Section "Core Files" SecCore
  SectionIn RO  ; Read-only section, always installed

  SetOutPath "$INSTDIR"

  ; Client executable
  File "..\build\r-type_client.exe"

  ; Server executable (optional, but included)
  File "..\build\r-type_server.exe"

  ; Plugins directory
  SetOutPath "$INSTDIR\plugins"
  File /r "..\build\plugins\*.dll"

  ; Assets directory (from build/assets which contains all game assets)
  SetOutPath "$INSTDIR\assets"
  File /r "..\build\assets\*.*"

  ; DLL dependencies from build directory (non-recursive to avoid copying subdirs)
  SetOutPath "$INSTDIR"
  File "..\build\*.dll"

  ; Copy icon file for shortcuts
  File "client.ico"

  ; Store installation folder
  WriteRegStr HKCU "Software\R-Type" "" $INSTDIR

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Create shortcuts (SetOutPath sets the working directory for shortcuts)
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\R-Type"
  CreateShortcut "$SMPROGRAMS\R-Type\R-Type Client.lnk" "$INSTDIR\r-type_client.exe" "" "$INSTDIR\client.ico" 0 SW_SHOWNORMAL "" "R-Type Client"
  CreateShortcut "$SMPROGRAMS\R-Type\R-Type Server.lnk" "$INSTDIR\r-type_server.exe" "" "$INSTDIR\client.ico" 0 SW_SHOWNORMAL "" "R-Type Server"
  CreateShortcut "$SMPROGRAMS\R-Type\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\R-Type.lnk" "$INSTDIR\r-type_client.exe" "" "$INSTDIR\client.ico" 0 SW_SHOWNORMAL "" "R-Type"

  ; Add to Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "DisplayName" "R-Type Game"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "DisplayIcon" "$INSTDIR\client.ico"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "Publisher" "EPITECH"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "DisplayVersion" "1.0.0"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type" "NoRepair" 1

SectionEnd

Section "Visual C++ Redistributable" SecVCRedist
  ; Check if Visual C++ Redistributable is needed
  SetOutPath "$TEMP"

  DetailPrint "Checking for Visual C++ Redistributable..."

  ; Try to download and install VC++ Redistributable
  ; This is optional and can be included in the installer package
  ; File "vc_redist.x64.exe"
  ; ExecWait '"$TEMP\vc_redist.x64.exe" /quiet /norestart'

SectionEnd

;--------------------------------
; Descriptions

; Language strings
LangString DESC_SecCore ${LANG_ENGLISH} "Core game files (required)"
LangString DESC_SecCore ${LANG_FRENCH} "Fichiers principaux du jeu (requis)"
LangString DESC_SecVCRedist ${LANG_ENGLISH} "Visual C++ Runtime (required for running the game)"
LangString DESC_SecVCRedist ${LANG_FRENCH} "Visual C++ Runtime (requis pour exécuter le jeu)"

; Assign descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVCRedist} $(DESC_SecVCRedist)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section

Section "Uninstall"

  ; Remove files
  Delete "$INSTDIR\r-type_client.exe"
  Delete "$INSTDIR\r-type_server.exe"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\client.ico"

  ; Remove plugins
  RMDir /r "$INSTDIR\plugins"

  ; Remove assets
  RMDir /r "$INSTDIR\assets"

  ; Remove extra directories (from File /r)
  RMDir /r "$INSTDIR\installer-package"
  RMDir /r "$INSTDIR\vcpkg_installed"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\R-Type\R-Type Client.lnk"
  Delete "$SMPROGRAMS\R-Type\R-Type Server.lnk"
  Delete "$SMPROGRAMS\R-Type\Uninstall.lnk"
  Delete "$DESKTOP\R-Type.lnk"
  RMDir "$SMPROGRAMS\R-Type"

  ; Remove installation directory
  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey HKCU "Software\R-Type"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\R-Type"

SectionEnd

;--------------------------------
; Functions

Function .onInit
  ; Check if running on 64-bit Windows
  ${If} ${RunningX64}
    ; 64-bit system
  ${Else}
    MessageBox MB_OK|MB_ICONSTOP "This application requires a 64-bit version of Windows."
    Abort
  ${EndIf}
FunctionEnd

Function LaunchGame
  ; Launch the game with the correct working directory
  SetOutPath "$INSTDIR"
  Exec "$INSTDIR\r-type_client.exe"
FunctionEnd