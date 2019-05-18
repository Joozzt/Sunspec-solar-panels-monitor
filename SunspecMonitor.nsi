; LuciLiveSE.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install LuciLive.nsi into a directory that the user selects,

;--------------------------------

;---- multi user support 5/1/16
; usage for silent installer LuciLive.exe /S /AllUsers or /CurrentUser 
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!include MultiUser.nsh
;----
!include "MUI2.nsh"

!define APPNAME "SunspecMonitor"
!define FRIENDLYAPPNAME "Sunspec Monitor"
!define COMPANYNAME "OMT"
!define DESCRIPTION ""
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0
# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links in here to open the email client
#!define HELPURL "http://www.luci.eu/?page_id=191" # "Support Information" link
#!define UPDATEURL "http://www.luci.eu/?page_id=1516" # "Product Updates" link
#!define ABOUTURL "http://www.luci.eu" # "Publisher" link
# This is the size (in kB) of all the files copied into "Program Files"
!define INSTALLSIZE 49200



; The name of the installer
Name "${APPNAME}"

; The file to write
!define OUTFILE "${APPNAME}_${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.exe"
OutFile ${OUTFILE}

#!finalize 'Luci.sign.bat "%1"'

; The default installation directory
InstallDir "$PROGRAMFILES64\${FRIENDLYAPPNAME}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${FRIENDLYAPPNAME}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

!define MUI_ICON "orange-install.ico"
!define MUI_UNICON "orange-uninstall.ico"
;---- Launch a program as user from UAC elevated installer 28-2-18
#!define MUI_FINISHPAGE_RUN "$INSTDIR\${APPNAME}.exe"
!define MUI_FINISHPAGE_RUN "$WINDIR\explorer.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start ${FRIENDLYAPPNAME}"
!define MUI_FINISHPAGE_RUN_PARAMETERS "$INSTDIR\${APPNAME}.exe" ;.lnk doesn't work in 64-bit windows 7... don't know why... "$SMPROGRAMS\${FRIENDLYAPPNAME}.lnk"

;--------------------------------

; Pages

#!insertmacro  MUI_PAGE_WELCOME
!insertmacro  MUI_PAGE_LICENSE "eulasunspecmonitor.rtf"
;---- multi user support 5/1/16
!insertmacro MULTIUSER_PAGE_INSTALLMODE
;----
#!insertmacro  MUI_PAGE_COMPONENTS
!insertmacro  MUI_PAGE_DIRECTORY
 #MUI_PAGE_STARTMENU pageid variable
!insertmacro  MUI_PAGE_INSTFILES
!insertmacro  MUI_PAGE_FINISH

#!insertmacro  MUI_UNPAGE_WELCOME
!insertmacro  MUI_UNPAGE_CONFIRM
 #MUI_UNPAGE_LICENSE textfile
 #MUI_UNPAGE_COMPONENTS
 #MUI_UNPAGE_DIRECTORY
!insertmacro  MUI_UNPAGE_INSTFILES
#!insertmacro  MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

;---- multi user support 5/1/16
Function .onInit
  !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd
;----
; The stuff to install
Section "${APPNAME}" SecDummy

  SectionIn RO
    !define SOURCEFILES build-Sunspec-Release\release

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
;Check if we have redistributable installed
Call  CheckRedistributableInstalled
Pop $R0

${If} $R0 != "1"
  File "${SOURCEFILES}\vc_redist.x64.exe" 	
  ExecWait '"$INSTDIR\vc_redist.x64.exe"  /passive /norestart'	
${EndIf}
  ; Put file there
;  File "${APPNAME}.nsi"
  File "${SOURCEFILES}\${APPNAME}.exe"
  File "sunspec\rc\icon.ico"
  File "orange-install.ico"
  File "orange-uninstall.ico"
  File "${SOURCEFILES}\Qt5Core.dll"
  File "${SOURCEFILES}\Qt5Gui.dll"
  File "${SOURCEFILES}\Qt5Network.dll"
  File "${SOURCEFILES}\Qt5Svg.dll"
  File "${SOURCEFILES}\Qt5Widgets.dll"
  File "SSLlibs\x64\libeay32.dll"
  File "SSLlibs\x64\ssleay32.dll"
	  
  SetOutPath $INSTDIR\platforms
  File "${SOURCEFILES}\platforms\qwindows.dll"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\${FRIENDLYAPPNAME}" "Install_Dir" "$INSTDIR"
  
	# Registry information for add/remove programs
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "DisplayName" "${FRIENDLYAPPNAME}${DESCRIPTION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "DisplayIcon" "$\"$INSTDIR\icon.ico$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "Publisher" "${COMPANYNAME}"
#	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "HelpLink" "${HELPURL}"
#	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "URLUpdateInfo" "${UPDATEURL}"
#	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "URLInfoAbout" "${ABOUTURL}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "VersionMajor" ${VERSIONMAJOR}
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "VersionMinor" ${VERSIONMINOR}
	# There is no option for modifying or repairing the install
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "NoRepair" 1
	# Set the INSTALLSIZE constant (!defined at the top of this script) so Add/Remove Programs can accurately report the size
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "EstimatedSize" ${INSTALLSIZE}
	WriteRegStr HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}" "DisplayName" "${FRIENDLYAPPNAME}${DESCRIPTION}"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
   SetOutPath $INSTDIR
  CreateShortCut "$SMPROGRAMS\${FRIENDLYAPPNAME}.lnk" "$INSTDIR\${APPNAME}.exe" "" "$INSTDIR\icon.ico"
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FRIENDLYAPPNAME}"
  DeleteRegKey HKLM "SOFTWARE\${FRIENDLYAPPNAME}"

  ; Remove files and uninstaller
  Delete "$INSTDIR\*.*"
  Delete "$INSTDIR\platforms\*.*"


  ; Remove directories used
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR"

;---- multi user support 5/1/16
  ; Remove shortcuts, if any ( unistaller does not know user choice so do both )
  SetShellVarContext all
  Delete "$SMPROGRAMS\${FRIENDLYAPPNAME}.lnk"
  SetShellVarContext current
  Delete "$SMPROGRAMS\${FRIENDLYAPPNAME}.lnk"
 ;----

SectionEnd

Function CheckRedistributableInstalled

  ;{F0C3E5D1-1ADE-321E-8167-68EF0DE699A5} - msvs2010 sp1
  
  Push $R0
  ClearErrors
   
  ;try to read Version subkey to R0
  ReadRegDword $R0 HKLM "Software\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"

  ;was there error or not?
  IfErrors 0 NoErrors
   
  ;error occured, copy "Error" to R0
  StrCpy $R0 "Error"

  NoErrors:
  
    Exch $R0 
FunctionEnd

