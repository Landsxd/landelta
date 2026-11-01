; Per-user installation. Keep this AppId stable across future upgrades.
#define AppVersion "0.13.3"
[Setup]
AppId={{B0498E99-D687-4838-A85F-372EF9BBCA40}
AppName=LANDELTA
AppVersion={#AppVersion}
AppPublisher=LANDELTA
AppPublisherURL=https://github.com/Landsxd/landelta
DefaultDirName={localappdata}\Programs\LANDELTA
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
AppMutex=Local\VortexACCOverlay
SetupMutex=Local\LANDELTASetup
CloseApplications=no
RestartApplications=no
OutputDir=..\dist
OutputBaseFilename=LANDELTA-Setup-{#AppVersion}
SetupIconFile=..\src\app.ico
UninstallDisplayIcon={app}\LANDELTA.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
spanish.SetupAppRunningError=LANDELTA sigue abierta, posiblemente junto al reloj.%n%nEn su icono, pulsa Salir de LANDELTA (la X solo la oculta). Después pulsa Aceptar para instalar la nueva versión.%n%nTus ajustes, historial y telemetría se conservan.

[Files]
Source: "..\LANDELTA.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\tools\ac-analyzer.html"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\licenses\*.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "..\vendor\JSON-LICENSE.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion

Source: "..\tools\session-analysis.js"; DestDir: "{app}\tools"; Flags: ignoreversion

[Icons]
Name: "{userprograms}\LANDELTA"; Filename: "{app}\LANDELTA.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\LANDELTA"; Filename: "{app}\LANDELTA.exe"; WorkingDir: "{app}"
Name: "{userstartup}\LANDELTA-Auto"; Filename: "{app}\LANDELTA.exe"; Parameters: "--background"; WorkingDir: "{app}"; Check: KeepStartup

[InstallDelete]
Type: files; Name: "{userstartup}\VortexACC-Auto.lnk"; Check: KeepStartup

[Run]
Filename: "{app}\LANDELTA.exe"; Description: "Abrir LANDELTA"; Flags: nowait postinstall skipifsilent

[Code]
var
  HadStartup: Boolean;

function InitializeSetup(): Boolean;
begin
  HadStartup := FileExists(ExpandConstant('{userstartup}\LANDELTA-Auto.lnk')) or
    FileExists(ExpandConstant('{userstartup}\VortexACC-Auto.lnk'));
  Result := True;
end;

function KeepStartup(): Boolean;
begin
  Result := HadStartup;
end;
