; Windows installer for Pressura (Inno Setup 6.3+)
;   iscc /DAppVersion=0.1.0 tools\installer\pressura.iss
; Run from the repo root after a Release build. Output: build\installer\Pressura-<version>-Windows-Setup.exe

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#define ArtDir "..\..\build\OPTO2_artefacts\Release"

[Setup]
AppId={{697E7089-65A4-4745-B7A8-062AF7A0752B}
AppName=Pressura
AppVersion={#AppVersion}
AppPublisher=gueibrielon
DefaultDirName={autopf}\Pressura
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputDir=..\..\build\installer
OutputBaseFilename=Pressura-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
InfoBeforeFile=welcome.txt
UninstallDisplayName=Pressura

[Types]
Name: "full"; Description: "VST3 plugin and Standalone app"
Name: "custom"; Description: "Choose"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (Ableton, FL Studio, Reaper, Cubase, Bitwig...)"; Types: full custom
Name: "app"; Description: "Standalone app"; Types: full

[Files]
Source: "{#ArtDir}\VST3\Pressura.vst3\*"; DestDir: "{commoncf64}\VST3\Pressura.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\Pressura.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "..\..\Resources\OFL.txt"; DestDir: "{app}"; DestName: "license-font.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Pressura"; Filename: "{app}\Pressura.exe"; Components: app
