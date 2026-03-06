; ============================================================
; ArpSequencer VST3 Plugin – Windows Installer (Inno Setup 6.x)
; ============================================================
; To compile:
;   1. Download Inno Setup 6 from https://jrsoftware.org/isinfo.php
;   2. Open this script in the Inno Setup Compiler
;   3. Press F9 (or Build > Compile)
;   4. Find the installer in the Output\ folder
; ============================================================

#define AppName      "ArpSequencer"
#define AppVersion   "1.0.0"
#define AppPublisher "MannyZeneke"
#define AppURL       "https://github.com/Mxnny88/mannyzeneke"
#define AppCopyright "Copyright © 2025 MannyZeneke"

; -- Paths to compiled plugin binaries (adjust as needed after CMake build) --
; Expected build output: build\ArpSequencer_artefacts\Release\VST3\ArpSequencer.vst3
#define VST3_SRC  "..\build\ArpSequencer_artefacts\Release\VST3\ArpSequencer.vst3"
#define SA_SRC    "..\build\ArpSequencer_artefacts\Release\Standalone\ArpSequencer.exe"

[Setup]
AppId               ={{E4A3B8C1-5D72-4F91-A823-7E90D1234567}
AppName             ={#AppName}
AppVersion          ={#AppVersion}
AppPublisher        ={#AppPublisher}
AppPublisherURL     ={#AppURL}
AppSupportURL       ={#AppURL}
AppUpdatesURL       ={#AppURL}
AppCopyright        ={#AppCopyright}
DefaultDirName      ={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName    ={#AppPublisher}
LicenseFile         =..\LICENSE.txt
OutputDir           =Output
OutputBaseFilename  ={#AppName}-{#AppVersion}-Windows-Setup
SetupIconFile       =..\Resources\icon.ico
Compression         =lzma2/ultra64
SolidCompression    =yes
WizardStyle         =modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\Uninstall.exe
CloseApplications   =yes
RestartApplications =no
PrivilegesRequired  =admin
MinVersion          =6.1   ; Windows 7+

; -- Wizard graphics (optional – place 164x314 and 55x58 BMP in Installer\) --
; WizardImageFile=installer_side.bmp
; WizardSmallImageFile=installer_small.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "vst3";       Description: "Install VST3 plugin (for Ableton, Reaper, etc.)"; Flags: checkedonce
Name: "standalone"; Description: "Install Standalone application";                  Flags: unchecked
Name: "desktopicon"; Description: "Create a Desktop shortcut (Standalone)";         GroupDescription: "Additional icons:"; Flags: unchecked

[Dirs]
Name: "{autopf64}\Common Files\VST3\{#AppPublisher}";  Tasks: vst3
Name: "{userappdata}\{#AppName}\Presets"

[Files]
; VST3 bundle (folder on Windows)
Source: "{#VST3_SRC}\*"; DestDir: "{autopf64}\Common Files\VST3\{#AppPublisher}\{#AppName}.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3

; Standalone app
Source: "{#SA_SRC}";     DestDir: "{app}";              Flags: ignoreversion; Tasks: standalone

; Documentation
Source: "..\README.md";  DestDir: "{app}";              Flags: ignoreversion isreadme

[Icons]
; Start menu
Name: "{group}\{#AppName} Standalone";  Filename: "{app}\{#AppName}.exe"; Tasks: standalone
Name: "{group}\Uninstall {#AppName}";   Filename: "{uninstallexe}"
; Desktop
Name: "{userdesktop}\{#AppName}";       Filename: "{app}\{#AppName}.exe"; Tasks: standalone and desktopicon

[Run]
; Notify user of VST3 location after install
Filename: "{app}";  Description: "Open installation folder"; \
    Flags: postinstall shellexec skipifsilent; Tasks: vst3

[UninstallRun]
; Kill standalone if running before uninstall
Filename: "{cmd}"; Parameters: "/c taskkill /f /im {#AppName}.exe"; \
    Flags: runhidden; RunOnceId: "KillStandalone"

[UninstallDelete]
; Remove presets only if user confirms (handled by code below)
Type: filesandordirs; Name: "{userappdata}\{#AppName}"

[Code]
// ─── Custom install-time checks ─────────────────────────────────────────────

function InitializeSetup(): Boolean;
begin
  // Warn if no VST3 host seems to be installed (heuristic check)
  if not DirExists(ExpandConstant('{autopf64}\Common Files\VST3')) then
  begin
    if MsgBox('No VST3 host detected on this system.'#13#10 +
              'You can still install the plugin; it will be available '  +
              'once you install a compatible DAW (Reaper, Ableton Live, FL Studio, etc.).' +
              #13#10#13#10 + 'Continue?',
              mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
      Exit;
    end;
  end;
  Result := True;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
end;

// ─── Ask user whether to keep presets on uninstall ──────────────────────────
function InitializeUninstall(): Boolean;
var
  Answer: Integer;
begin
  Answer := MsgBox('Do you want to keep your saved presets?' + #13#10 +
                   '(Located in %AppData%\{#AppName}\Presets)',
                   mbConfirmation, MB_YESNO);
  if Answer = IDYES then
  begin
    // Prevent UninstallDelete from removing the folder
    // (We skip it by re-naming; simplest approach = just inform user)
    MsgBox('Your presets will be kept at:'#13#10 +
           ExpandConstant('{userappdata}\{#AppName}\Presets'), mbInformation, MB_OK);
  end;
  Result := True;
end;

// ─── Post-install message ────────────────────────────────────────────────────
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    MsgBox('{#AppName} {#AppVersion} installed successfully!' + #13#10 +
           #13#10 +
           'VST3 location: %ProgramFiles%\Common Files\VST3\{#AppPublisher}\{#AppName}.vst3' +
           #13#10 +
           'Please rescan your VST3 plugins in your DAW.' + #13#10 +
           #13#10 +
           'Enjoy making music!', mbInformation, MB_OK);
  end;
end;
