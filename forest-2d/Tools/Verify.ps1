param(
    [ValidateSet('Build','Tests','Replay','Package')][string]$Action='Tests',
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$Archive
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$projectFile=Join-Path $projectRoot 'Forest2D.uproject'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
switch ($Action) {
    'Build' {
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') Forest2DEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
    }
    'Tests' {
        & $editor $projectFile -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests Forest2D' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$projectRoot/Saved/Automation" -stdout
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $result=Get-Content "$projectRoot/Saved/Automation/index.json" -Raw | ConvertFrom-Json
        if ($result.failed -ne 0 -or $result.succeeded -ne 4) { throw 'Forest2D regression tests did not all pass.' }
    }
    'Replay' {
        & $editor $projectFile -game -RenderOffscreen -ResX=1440 -ResY=900 -NoSound -ForestReplay -ForestReplayExit -ForestCapture -unattended -stdout
    }
    'Package' {
        if (-not $Archive) { throw 'Specify -Archive for the packaged game.' }
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$Archive" -utf8output
    }
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
