#define PluginName "Appeal_SDL2_Input"
#define PluginVersion "1.0.0-beta.1"

[Setup]
AppId={{5DD81FD1-B8CC-4ECA-9E8A-5ED7098B21A1}
AppName=Appeal SDL2 Input Plugin
AppVersion={#PluginVersion}
AppPublisher=Red Whistle
DefaultDirName={localappdata}\Programs\Appeal Plugins\{#PluginName}
DisableProgramGroupPage=yes
OutputDir=..\..\Dist
OutputBaseFilename={#PluginName}-{#PluginVersion}-windows-x64-installer
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Files]
Source: "..\..\{#PluginName}.dll"; DestDir: "{code:GetPluginDirectory}"; Flags: ignoreversion

[UninstallDelete]
Type: files; Name: "{code:GetPluginDirectory}\SDL2.dll"
Type: dirifempty; Name: "{code:GetPluginDirectory}"

[Code]
const
  RuntimeURL = 'https://github.com/libsdl-org/SDL/releases/download/release-2.24.2/SDL2-2.24.2-win32-x64.zip';
  RuntimeArchive = 'SDL2-2.24.2-win32-x64.zip';
  RuntimeSHA256 = 'd064e25ab265f288586966137abf356cac698522dc05b788d178d9abf2e0462d';

function GetResourcesDirectory: String;
var
  OverridePath: String;
begin
  OverridePath := ExpandConstant('{param:APPEALRESOURCES|}');
  if OverridePath <> '' then Result := OverridePath
  else Result := ExpandConstant('{userdocs}\Appeal\Resources');
end;

function GetPluginDirectory(Param: String): String;
begin
  Result := GetResourcesDirectory + '\Modules\Input\{#PluginName}';
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Shell, ZipFolder, TargetFolder: Variant;
  ZipPath, TargetPath: String;
begin
  if CurStep <> ssInstall then Exit;
  DownloadTemporaryFile(RuntimeURL, RuntimeArchive, RuntimeSHA256, nil);
  ZipPath := ExpandConstant('{tmp}\' + RuntimeArchive);
  TargetPath := GetPluginDirectory('');
  ForceDirectories(TargetPath);
  Shell := CreateOleObject('Shell.Application');
  ZipFolder := Shell.NameSpace(ZipPath);
  TargetFolder := Shell.NameSpace(TargetPath);
  if VarIsClear(ZipFolder) or VarIsClear(TargetFolder) then
    RaiseException('Unable to extract the SDL2 runtime archive.');
  TargetFolder.CopyHere(ZipFolder.Items, 4 or 16);
end;
