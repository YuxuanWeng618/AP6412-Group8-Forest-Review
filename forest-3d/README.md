# Forest Together · 3D reconstruction and play

The team's C++ and C# additions are in [overlay/](overlay/). Changes to existing Project Becorn source and configuration are in [ProjectBecorn.patch](ProjectBecorn.patch). The setup script assembles the project locally from the [pinned Project Becorn revision](https://github.com/LeoSery/Project-Becorn--UnrealEngine5-2024/tree/d35718cfe66854e0a9b40ea06d44e4ce4fa44b1f).

## 1. Prepare the project

Use Windows x64, Git for Windows, Unreal Engine 5.8.1, the Visual Studio C++ toolchain and Windows SDK, and a standalone [x64 .NET 10 SDK](https://dotnet.microsoft.com/en-us/download/dotnet/10.0) from the 10.0.4xx feature band. UnrealSharp's source generators require a newer C# compiler than the one in the engine's 10.0.203 SDK. Put the repository in a short ASCII path such as `C:\ForestReview`. The first step downloads about 1.4 GB of upstream sources and assets. Allow extra disk space for the download, extraction and build.

From the repository root in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\Prepare-3D.ps1
```

To reuse the ZIP downloaded from the pinned revision, add `-UpstreamZip C:\Downloads\ProjectBecorn.zip`. The default output is `build-3d/`. For a new reconstruction, choose a fresh directory with `-OutDir`.

## 2. Build

From the repository root, set the first two paths to your installation. If you used a custom output directory, change the `$project` path accordingly.

```powershell
$engine = 'C:\UE_5.8'
$sdk = 'C:\Program Files\dotnet'
if (-not (& "$sdk\dotnet.exe" --list-sdks | Select-String '^10\.0\.4\d\d\s')) {
    throw 'Set $sdk to a standalone x64 .NET 10.0.4xx SDK directory.'
}
$project = (Resolve-Path '.\build-3d\BCR.uproject').Path
$projectRoot = Split-Path -Parent $project
$env:FOREST_SHARED_RULES = (Resolve-Path '.\forest-2d\Source\Forest2D\Domain').Path
$env:DOTNET_ROOT = $sdk
$env:DOTNET_ROOT_X64 = $sdk
$env:PATH = "$sdk;$env:PATH"
$env:DOTNET_MULTILEVEL_LOOKUP = '0'
$env:DOTNET_ROLL_FORWARD = 'LatestMajor'

$buildTool = "$engine\Engine\Binaries\DOTNET\UnrealBuildTool\UnrealBuildTool.dll"
& "$sdk\dotnet.exe" $buildTool BCREditor Win64 Development "-Project=$project" -WaitMutex -NoUBA
if ($LASTEXITCODE -ne 0) { throw 'BCREditor build failed.' }

$automation = "$engine\Engine\Binaries\DOTNET\AutomationTool"
Push-Location $automation
try {
    & "$sdk\dotnet.exe" "$automation\AutomationTool.dll" BuildUserSolution `
        "-ScriptDir=$projectRoot" "-ScriptsForProject=$project" "-Project=$project" `
        -TargetType=Editor -TargetConfiguration=Development `
        "-OutputPath=$projectRoot\Binaries\Managed\net10.0" -unattended
    if ($LASTEXITCODE -ne 0) { throw 'UnrealSharp managed build failed.' }
} finally { Pop-Location }

& "$engine\Engine\Binaries\Win64\UnrealEditor.exe" $project
```

## 3. Generate bilingual prompts and play

Before playing for the first time, select **Tools → forest: Apply English / 中文 Hints** in the editor. This applies the team's translations and widget changes to the locally downloaded assets. The plugin imports the CJK font from the installed engine. If your engine installation lacks that font, set `BECORN_CJK_FONT` to the full path of a local CJK `.ttf` or `.otf` before launching the editor.

Open `/Game/Becorn/Maps/Map_Blocking_V2`, choose **Play**, click the viewport and press **Space**. Press **H** to see both players' keys.

For the standalone managed rules tests, run `dotnet run --project tests/ManagedRules` from the repository root using the .NET 8 SDK. See the [Route B comparison](../docs/ROUTE_B_COMPARISON.md) for the shared rules and verification scope.
