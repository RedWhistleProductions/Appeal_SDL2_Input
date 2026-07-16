param([switch]$Force)

$ErrorActionPreference = 'Stop'
$Root = Resolve-Path "$PSScriptRoot\.."
$Manifest = Get-Content "$Root\dependencies.json" -Raw | ConvertFrom-Json
$Dependency = $Manifest.dependencies.SDL2
$Deps = Join-Path $Root '.deps'
$Cache = Join-Path $Deps 'cache'
$Destination = Join-Path $Deps 'windows-x64'
$Archive = Join-Path $Cache 'SDL2-devel-mingw.tar.gz'
$Extracted = Join-Path $Deps 'extract-SDL2'

if((Test-Path "$Destination\include\SDL2\SDL.h") -and -not $Force) {
    Write-Host "SDL2 development files are already installed in $Destination"
    exit 0
}

New-Item -ItemType Directory -Force $Cache | Out-Null
Invoke-WebRequest $Dependency.windows.development_url -OutFile $Archive
$ActualHash = (Get-FileHash $Archive -Algorithm SHA256).Hash
if($ActualHash -ne $Dependency.windows.development_sha256) {
    throw "SDL2 checksum mismatch. Expected $($Dependency.windows.development_sha256), received $ActualHash."
}

if(Test-Path $Extracted) { Remove-Item $Extracted -Recurse -Force }
New-Item -ItemType Directory -Force $Extracted | Out-Null
tar -xzf $Archive -C $Extracted
$Toolchain = Get-ChildItem $Extracted -Directory -Recurse |
    Where-Object {
        $_.FullName -match 'x86_64-w64-mingw32' -and
        (Test-Path (Join-Path $_.FullName 'include\SDL2\SDL.h'))
    } |
    Select-Object -First 1
if(-not $Toolchain) { throw 'The SDL2 MinGW toolchain directory was not found in the downloaded archive.' }

if(Test-Path $Destination) { Remove-Item $Destination -Recurse -Force }
New-Item -ItemType Directory -Force $Destination | Out-Null
Copy-Item "$($Toolchain.FullName)\include" $Destination -Recurse
Copy-Item "$($Toolchain.FullName)\lib" $Destination -Recurse
Copy-Item "$($Toolchain.FullName)\bin" $Destination -Recurse
Remove-Item $Extracted -Recurse -Force
Write-Host "Installed SDL2 $($Dependency.version) development files in $Destination"
