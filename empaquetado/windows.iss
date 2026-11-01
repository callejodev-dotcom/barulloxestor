; Instalador de Barullo Xestor para Windows (Inno Setup 6.5 ou posterior, en galego).
; Instálase para o usuario (sen admin): o xestor só pide o permiso (UAC) cando ten que quitar
; un plugin de C:\Program Files\Common Files.
; ISCC /DAppVersion=0.2.0 /DArtDir=<carpeta co .exe> /DOutDir=<saída> /DLangDir=<carpeta con Galician.isl> windows.iss
#define Nome "Barullo Xestor"

[Setup]
AppId={{6B1C2E0A-4D7F-4F3B-9A51-2E8C7D9F0B14}
AppName={#Nome}
AppVersion={#AppVersion}
AppPublisher=Escoita
AppPublisherURL=https://www.escoita.link
DefaultDirName={localappdata}\Programs\{#Nome}
DefaultGroupName={#Nome}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#OutDir}
OutputBaseFilename=BarulloXestor-{#AppVersion}-windows-x64-instalador
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\{#Nome}.exe
WizardStyle=modern

[Languages]
Name: "gl"; MessagesFile: "{#LangDir}\Galician.isl"

[Files]
Source: "{#ArtDir}\{#Nome}.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{userprograms}\{#Nome}"; Filename: "{app}\{#Nome}.exe"

[Run]
Filename: "{app}\{#Nome}.exe"; Description: "Abrir {#Nome}"; Flags: nowait postinstall skipifsilent
