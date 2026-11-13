[CmdletBinding()]
param(
    [string]$UpstreamZip,
    [string]$OutDir = (Join-Path $PSScriptRoot 'build-3d')
)

$ErrorActionPreference = 'Stop'
$commit = 'd35718cfe66854e0a9b40ea06d44e4ce4fa44b1f'
$upstream = 'Project-Becorn--UnrealEngine5-2024'
$overlay = Join-Path $PSScriptRoot 'forest-3d/overlay'
$patch = Join-Path $PSScriptRoot 'forest-3d/ProjectBecorn.patch'
$target = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
if (Test-Path -LiteralPath $target) { throw "Choose an output directory that does not exist: $target" }
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git for Windows is required.' }

Add-Type -AssemblyName System.IO.Compression.FileSystem
$downloaded = $false
$archive = $null
$stage = Join-Path (Split-Path -Parent $target) ('.forest-3d-' + [guid]::NewGuid())
try {
    if ($UpstreamZip) {
        $UpstreamZip = (Resolve-Path -LiteralPath $UpstreamZip).Path
    } else {
        $UpstreamZip = Join-Path ([IO.Path]::GetTempPath()) ("Forest-Becorn-$([guid]::NewGuid()).zip")
        $downloaded = $true
        Write-Host 'Downloading the pinned Project Becorn archive (about 1.4 GB).'
        & curl.exe --fail --location --retry 2 --output $UpstreamZip "https://codeload.github.com/LeoSery/$upstream/zip/$commit"
        if ($LASTEXITCODE -ne 0) { throw 'Project Becorn download failed.' }
    }

    $archive = [IO.Compression.ZipFile]::OpenRead($UpstreamZip)
    $prefix = "$upstream-$commit/"
    $utf8 = [Text.UTF8Encoding]::new($false)
    $patchText = [IO.File]::ReadAllText($patch, $utf8)
    $patched = @{}
    foreach ($match in [regex]::Matches($patchText, '(?m)^--- a/([^\r\n]+)')) {
        $patched[$match.Groups[1].Value] = $true
    }
    $assetCount = 0
    foreach ($entry in $archive.Entries) {
        if (-not $entry.Name -or -not $entry.FullName.StartsWith($prefix, [StringComparison]::Ordinal)) { continue }
        $relative = $entry.FullName.Substring($prefix.Length)
        if ($relative -ne 'BCR.uproject' -and $relative -notmatch '^(Config|Source|Content)/|^Plugins/RokokoIKRigRetargeting-main/') { continue }
        if ($relative -match '(^|/)\.\.?(/|$)|[:\\]') { throw "Invalid archive path: $relative" }
        $path = Join-Path $stage $relative
        [IO.Directory]::CreateDirectory((Split-Path -Parent $path)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $path)
        if ($patched.ContainsKey($relative)) {
            $text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n").Replace("`r", "`n")
            if (-not $text.EndsWith("`n")) { $text += "`n" }
            [IO.File]::WriteAllText($path, $text, $utf8)
        }
        if ($relative.StartsWith('Content/')) { $assetCount++ }
    }
    if ($assetCount -ne 2902 -or -not (Test-Path -LiteralPath (Join-Path $stage 'BCR.uproject'))) {
        throw 'The supplied archive is missing files from the pinned Project Becorn revision.'
    }
    & git -C $stage apply --unidiff-zero --check $patch
    if ($LASTEXITCODE -ne 0) { throw 'Project Becorn source patch check failed.' }
    & git -C $stage apply --unidiff-zero $patch
    if ($LASTEXITCODE -ne 0) { throw 'Project Becorn source patch could not be applied.' }
    foreach ($item in Get-ChildItem -LiteralPath $overlay -Force) {
        Copy-Item -LiteralPath $item.FullName -Destination $stage -Recurse -Force
    }
    Move-Item -LiteralPath $stage -Destination $target
    Write-Host "Prepared the 3D project with $assetCount upstream content files at $target"
} finally {
    if ($null -ne $archive) { $archive.Dispose() }
    if ($downloaded -and (Test-Path -LiteralPath $UpstreamZip)) {
        Remove-Item -LiteralPath $UpstreamZip -Force
    }
    if (Test-Path -LiteralPath $stage) {
        $resolved = (Resolve-Path -LiteralPath $stage).Path
        if ($resolved -eq $stage -and (Split-Path -Leaf $resolved) -match '^\.forest-3d-[0-9a-f-]{36}$') {
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
    }
}
