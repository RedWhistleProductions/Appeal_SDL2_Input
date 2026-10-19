param(
    [switch]$Build,
    [string]$DevelopmentRoot,
    [string]$AppealResources,
    [string]$HostDirectory
)
$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$Manifest = Get-Content (Join-Path $Root 'plugin.json') -Raw | ConvertFrom-Json
if (-not $HostDirectory) { $HostDirectory = [IO.Path]::GetFullPath((Join-Path $Root '../../../..')) }
if (-not (Test-Path (Join-Path $HostDirectory 'Appeal.exe'))) { throw 'Specify -HostDirectory containing Appeal.exe.' }
if (-not $AppealResources) { $AppealResources = Join-Path $HostDirectory 'Resources' }
$Deps = Join-Path $Root '.deps/windows-x64'
if ($DevelopmentRoot) {
    $DevelopmentRoot = (Resolve-Path $DevelopmentRoot).Path
    foreach ($File in @('include/SDL2/SDL.h','lib/libSDL2.dll.a','bin/SDL2.dll')) {
        if (-not (Test-Path (Join-Path $DevelopmentRoot $File))) { throw "Missing SDL2 development file: $File" }
    }
    New-Item -ItemType Directory -Force (Join-Path $Deps 'include'),(Join-Path $Deps 'lib'),(Join-Path $Deps 'bin') | Out-Null
    Copy-Item (Join-Path $DevelopmentRoot 'include/SDL2') (Join-Path $Deps 'include') -Recurse -Force
    Copy-Item (Join-Path $DevelopmentRoot 'lib/libSDL2.dll.a') (Join-Path $Deps 'lib') -Force
    Copy-Item (Join-Path $DevelopmentRoot 'bin/SDL2.dll') (Join-Path $Deps 'bin') -Force
}
if ($Build) {
    if (-not (Test-Path (Join-Path $Deps 'include/SDL2/SDL.h'))) {
        & (Join-Path $Root 'Scripts/Setup-Dependencies.ps1')
    }
    Push-Location $Root
    try {
        & make -f (Join-Path $Root 'Makefile') -j4
        if ($LASTEXITCODE -ne 0) { throw 'Input plugin build failed.' }
    } finally { Pop-Location }
}
$Artifact = Join-Path $Root $Manifest.artifacts.windows
if (-not (Test-Path $Artifact)) { throw "Missing artifact: $Artifact. Run with -Build." }
$Destination = Join-Path $AppealResources "Modules/Appeal_Module_$($Manifest.module)/$($Manifest.name)"
New-Item -ItemType Directory -Force $Destination | Out-Null
if ([IO.Path]::GetFullPath($Destination) -ne $Root) { Copy-Item $Artifact $Destination -Force }
$Runtime = Join-Path $Deps 'bin/SDL2.dll'
$Installed = Join-Path $HostDirectory 'SDL2.dll'
if (-not (Test-Path $Installed)) {
    if (-not (Test-Path $Runtime)) { throw "Missing SDL2 runtime: $Runtime" }
    Copy-Item $Runtime $Installed
} else {
    Write-Host 'Keeping the existing shared SDL2 runtime beside Appeal.exe.'
}
Write-Host "Installed $($Manifest.name) in $Destination"
