; Links Windows installer script

#define MyAppName "Links"
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#define MyAppPublisher "Sqhh99"
#define MyAppURL "https://github.com/Sqhh99/links"
#define MyAppExeName "links.exe"

#ifndef SourceRoot
  #define SourceRoot "..\.."
#endif
#ifndef DistDir
  #define DistDir "..\..\dist\Links"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist\installer"
#endif

[Setup]
AppId={{8A7D4F1E-3D5D-4B7B-9D47-83D3E0B85E58}}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
LicenseFile={#SourceRoot}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=Links-v{#AppVersion}-win-x64-setup
SetupIconFile={#SourceRoot}\res\icon\appIcon\windows\icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#DistDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
var
  RemoveUserDataOnUninstall: Boolean;

function ConfirmRemoveUserData(): Boolean;
begin
  if UninstallSilent then
  begin
    Result := False;
    exit;
  end;

  Result := MsgBox(
    'Do you also want to remove Links user data?' + #13#10 +
    'This includes local settings and cache under AppData.' + #13#10 +
    'Recorded meeting files will NOT be deleted.',
    mbConfirmation, MB_YESNO) = IDYES;
end;

procedure RemoveUserDataDirectories();
begin
  DelTree(ExpandConstant('{userappdata}\Links'), True, True, True);
  DelTree(ExpandConstant('{localappdata}\Links'), True, True, True);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    RemoveUserDataOnUninstall := ConfirmRemoveUserData();
  end
  else if (CurUninstallStep = usPostUninstall) and RemoveUserDataOnUninstall then
  begin
    RemoveUserDataDirectories();
  end;
end;
