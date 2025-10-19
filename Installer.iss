
; See Github: https://github.com/DomGries/InnoDependencyInstaller
#include "CodeDependencies.iss"

#define WinAudio_Name "WinAudio Player"
#define WinAudio_Exe  WinAudio_Name + ".exe"
#define WinAudio_Version "0.10"

[Setup]
AppName=WinAudio Player
AppVersion={#WinAudio_Version}
DefaultDirName={autopf64}\{#WinAudio_Name}
DefaultGroupName={#WinAudio_Name}
WizardStyle=classic
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible 
AppPublisherURL=https://github.com/MarcoBellini/WinAudio-Player
LicenseFile=LICENSE.txt
SetupIconFile=Icons\mainIcon.ico
UninstallDisplayIcon={app}\{#WinAudio_Exe}
OutputBaseFilename={#WinAudio_Name}-{#WinAudio_Version}
OutputDir={#SourcePath}\Setup
MinVersion=10.0

[Files]
source:"x64\Release\{#WinAudio_Exe}"; DestDir: "{app}"; Check: Dependency_IsX64
source:"x64\Release\*.dll"; DestDir: "{app}"
Source:"x64\Release\Plugins\*.dll"; DestDir:"{app}\Plugins"

[Icons]
Name:"{autodesktop}\{#WinAudio_Name}"; Filename: "{app}\{#WinAudio_Exe}" ; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"

[Run]
Filename: "{app}\{#WinAudio_Exe}"; Description: "{cm:LaunchProgram,{#WinAudio_Name}}"; Flags: nowait postinstall skipifsilent

[Code]
function InitializeSetup: Boolean;
begin
  Dependency_AddVC2015To2022;
  Result := True;
end;
