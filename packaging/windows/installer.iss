; Inno Setup 6 script for Artilody Sampleforge.
; Compile: ISCC.exe /DAppVersion=0.3.0 /DBuildDir=<cmake build dir> packaging\windows\installer.iss

#ifndef AppVersion
  #define AppVersion "0.3.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build-windows"
#endif
#define AppName "Artilody Sampleforge"
#define AppExe "Artilody Sampleforge.exe"

[Setup]
AppId={{6F1C9C2E-5B7A-4E39-9C1D-A7D1F0B5E2C4}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Artilody
AppPublisherURL=https://github.com/shenphenchoedron-hue/Artilody-Sampleforge
AppSupportURL=https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/issues
DefaultDirName={autopf}\Artilody Sampleforge
DefaultGroupName=Artilody Sampleforge
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=Artilody-Sampleforge-{#AppVersion}-Windows-x64-Setup
SetupIconFile={#BuildDir}\Sampleforge_artefacts\JuceLibraryCode\icon.ico
UninstallDisplayIcon={app}\{#AppExe}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
MinVersion=10.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BuildDir}\Sampleforge_artefacts\Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"
Source: "..\..\JUCE-LICENSE.md"; DestDir: "{app}"
Source: "..\..\INSTRUMENT-FORMAT.md"; DestDir: "{app}"
Source: "..\..\docs\Artilody-Sampleforge-Manual-EN.pdf"; DestDir: "{app}\docs"; Flags: skipifsourcedoesntexist
Source: "..\..\docs\Artilody-Sampleforge-Manual-DA.pdf"; DestDir: "{app}\docs"; Flags: skipifsourcedoesntexist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Manual (English)"; Filename: "{app}\docs\Artilody-Sampleforge-Manual-EN.pdf"
Name: "{group}\Manual (Dansk)"; Filename: "{app}\docs\Artilody-Sampleforge-Manual-DA.pdf"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
