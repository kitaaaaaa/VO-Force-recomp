# One-click setup: from a disc image to a built game. Run by "Play Virtual-On Force.bat".
#
# Everything is downloaded into .tools\ next to this project; nothing is installed on
# the system except Visual Studio's C++ tools, and those only after asking. Running it
# again picks up where it stopped.

param([string]$Iso)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Tools = Join-Path $Root '.tools'
$Assets = Join-Path $Root 'assets'
$Logs = Join-Path $Root 'logs'
$Exe = Join-Path $Root 'out\build\win-amd64-release\force.exe'

$XexSha256 = '9FE5B63F76A089804F8B18D7F53F285E2743BFE5D8B43ED9A7E863DAAD5148C0'
$StringsSha256 = 'DC2C7BC642360E250B1B564B7A9003ED52C4124DCBBDC7DB0C2BEB59559D2CE6'
$SdkTag = 'v0.10.0'
$SdkUrl = 'https://github.com/rexglue/rexglue-sdk.git'

# Python comes first: it unpacks LLVM.
$Downloads = @(
  @{ Name = 'python'; Check = 'python.exe'; Strip = 0
     Url = 'https://www.python.org/ftp/python/3.13.3/python-3.13.3-embed-amd64.zip'
     Sha256 = '59FF76E16E6597DE47474FB22BE69E7191A89116910D728AB735079B078E52DB' }
  @{ Name = 'llvm'; Check = 'bin\clang.exe'; Strip = 1
     Url = 'https://github.com/llvm/llvm-project/releases/download/llvmorg-23.1.2/clang+llvm-23.1.2-x86_64-pc-windows-msvc.tar.xz'
     Sha256 = '8FB91CDC44FCBBDCF6B3FFD0A1F9859ABD14A3C3AAE4423C2B6D4A4F90BF0095' }
  @{ Name = 'cmake'; Check = 'bin\cmake.exe'; Strip = 1
     Url = 'https://github.com/Kitware/CMake/releases/download/v4.4.4/cmake-4.4.4-windows-x86_64.zip'
     Sha256 = 'BACE36E94B31C68AB6FA295F26DFA11219E0701CF7C94B0284A7D1CB13DAC536' }
  @{ Name = 'ninja'; Check = 'ninja.exe'; Strip = 0
     Url = 'https://github.com/ninja-build/ninja/releases/download/v1.13.1/ninja-win.zip'
     Sha256 = '26A40FA8595694DEC2FAD4911E62D29E10525D2133C9A4230B66397774AE25BF' }
  @{ Name = 'git'; Check = 'cmd\git.exe'; Strip = 0
     Url = 'https://github.com/git-for-windows/git/releases/download/v2.51.0.windows.1/MinGit-2.51.0-64-bit.zip'
     Sha256 = 'C2C955A21FA99889D83F485F24FA5D9A38FFFC2D509D4022385510E11C26B250' }
)

function Step($text) { Write-Host ''; Write-Host "== $text" -ForegroundColor Cyan }

function Fail($text) { Write-Host ''; Write-Host $text -ForegroundColor Red; exit 1 }

# Native tools write progress to stderr, which must not count as a PowerShell error.
function Run($exe) {
  $ErrorActionPreference = 'Continue'
  & $exe @args
  if ($LASTEXITCODE -ne 0) { Fail "Failed ($LASTEXITCODE): $exe $args" }
}

function Get-Tool($d) {
  $dest = Join-Path $Tools $d.Name
  if (Test-Path (Join-Path $dest $d.Check)) { return }
  $archive = Join-Path $Tools (Split-Path -Leaf $d.Url)
  $temp = "$dest.unpack"
  if (-not ($d.Sha256 -and (Test-Path $archive) -and (Get-FileHash $archive -Algorithm SHA256).Hash -eq $d.Sha256)) {
    Write-Host "Downloading $($d.Name)..."
    Run curl.exe --location --fail --retry 3 --continue-at - --output $archive $d.Url
  }
  $hash = (Get-FileHash $archive -Algorithm SHA256).Hash
  if ($d.Sha256 -and $hash -ne $d.Sha256) {
    Remove-Item $archive -Force
    Fail "$($d.Name): the download does not match the expected checksum. Run setup again."
  }
  Write-Host "$($d.Name) SHA256 $hash"
  foreach ($old in @($temp, $dest)) { if (Test-Path $old) { Remove-Item $old -Recurse -Force } }
  New-Item -ItemType Directory $temp | Out-Null
  Write-Host "Unpacking $($d.Name)..."
  if ($archive.EndsWith('.tar.xz')) {
    Run (Join-Path $Tools 'python\python.exe') (Join-Path $PSScriptRoot 'untar.py') $archive $temp
  } else {
    Run tar.exe -xf $archive -C $temp "--strip-components=$($d.Strip)"
  }
  Move-Item $temp $dest
  Remove-Item $archive -Force
}

# With core.symlinks off, a link in the SDK's submodules is checked out as a text file
# holding its target. Symlinks need Developer Mode on Windows, so use copies instead.
function Copy-SymlinkTargets($repo) {
  foreach ($line in (& git -C $repo ls-files -s)) {
    if ($line -notmatch '^120000 \S+ \d+\t(.+)$') { continue }
    $link = Join-Path $repo $Matches[1]
    if (-not (Test-Path $link -PathType Leaf)) { continue }
    $target = Join-Path (Split-Path -Parent $link) ((Get-Content $link -Raw).Trim())
    if (Test-Path $target -PathType Leaf) { Copy-Item $target $link -Force }
  }
}

function Build-Sdk {
  $prefix = Join-Path $Tools 'rexsdk'
  if (Test-Path (Join-Path $prefix 'bin\rexglue.exe')) { return }
  $src = Join-Path $Tools 'sdk'
  $stamp = Join-Path $src '.force-patched'
  if (-not (Test-Path $stamp)) {
    if (Test-Path $src) { Remove-Item $src -Recurse -Force }
    Step "Downloading the ReXGlue SDK $SdkTag source"
    Run git clone --recursive --depth 1 --shallow-submodules --branch $SdkTag $SdkUrl $src
    foreach ($g in (Get-ChildItem (Join-Path $src 'thirdparty') -Recurse -Depth 3 -Force -Filter .git)) {
      Copy-SymlinkTargets $g.DirectoryName
    }
    foreach ($p in (Get-ChildItem (Join-Path $Root 'sdk-patches') -Filter *.patch | Sort-Object Name)) {
      Run git -C $src apply --whitespace=nowarn $p.FullName
    }
    New-Item -ItemType File $stamp | Out-Null
  }
  Step 'Building the ReXGlue SDK (the longest step)'
  Run (Join-Path $PSScriptRoot 'build_sdk.bat') $src $prefix
}

function Find-VisualStudio {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (-not (Test-Path $vswhere)) { return $null }
  return & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}

function Ensure-VisualStudio {
  $vs = Find-VisualStudio
  if ($vs) { return $vs }
  Write-Host 'The Microsoft C++ build tools are needed to build the game and were not found.'
  $answer = Read-Host 'Install Visual Studio 2022 Build Tools now? It is free, about 7 GB, and Windows will ask for permission. (y/n)'
  if ($answer -notmatch '^[yY]') {
    Fail 'Install "Desktop development with C++" from https://visualstudio.microsoft.com/vs/community/ and run this again.'
  }
  Run winget install --id Microsoft.VisualStudio.2022.BuildTools --source winget --accept-package-agreements --accept-source-agreements --override '--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
  $vs = Find-VisualStudio
  if (-not $vs) { Fail 'The C++ build tools are still not found. Restart Windows and run this again.' }
  return $vs
}

function Find-Iso {
  if ($Iso) { return $Iso }
  $found = @(Get-ChildItem $Root -Filter *.iso)
  if ($found.Count -eq 1) { return $found[0].FullName }
  Add-Type -AssemblyName System.Windows.Forms
  $dialog = New-Object System.Windows.Forms.OpenFileDialog
  $dialog.Title = 'Choose your Dennou Senki Virtual-On Force disc image'
  $dialog.Filter = 'Xbox 360 disc image (*.iso)|*.iso'
  if ($dialog.ShowDialog() -ne 'OK') { Fail 'No disc image chosen.' }
  return $dialog.FileName
}

function Install-Game {
  $xex = Join-Path $Assets 'default.xex'
  if (-not (Test-Path $xex)) {
    Step 'Extracting the game from your disc image'
    $image = Find-Iso
    Run (Join-Path $Tools 'python\python.exe') (Join-Path $PSScriptRoot 'xiso_extract.py') $image $Assets
    if (-not (Test-Path $xex)) { Fail 'That disc image has no default.xex.' }
  }
  if ((Get-FileHash $xex -Algorithm SHA256).Hash -ne $XexSha256) {
    Fail "assets\default.xex is not the Japanese retail Virtual-On Force this project was made from (SHA256 $XexSha256)."
  }
}

# The English text replaces media\string_array.bin; the original is kept beside it.
function Install-Translation {
  $strings = Join-Path $Assets 'media\string_array.bin'
  $original = Join-Path $Assets 'media\string_array.original.bin'
  if (Test-Path $original) { return }
  if ((Get-FileHash $strings -Algorithm SHA256).Hash -ne $StringsSha256) {
    Fail 'media\string_array.bin is not the original Japanese text.'
  }
  Step 'Applying the English text'
  $out = Join-Path $Tools 'string_array.bin'
  Run (Join-Path $Tools 'python\python.exe') (Join-Path $PSScriptRoot 'translation\build_strings.py') $strings $out
  Copy-Item $strings $original
  Copy-Item $out $strings -Force
}

New-Item -ItemType Directory -Force $Tools, $Logs | Out-Null
Start-Transcript -Path (Join-Path $Logs 'setup.log') -Append | Out-Null
$started = Get-Date

[Environment]::SetEnvironmentVariable('VSINSTALL', (Ensure-VisualStudio), 'Process')

Step 'Downloading the build tools'
foreach ($d in $Downloads) { Get-Tool $d }

$env:FORCE_TOOLS = (@('llvm\bin', 'cmake\bin', 'ninja', 'git\cmd') | ForEach-Object { Join-Path $Tools $_ }) -join ';'
$env:PATH = "$env:FORCE_TOOLS;$env:PATH"
# Files exactly as stored: the SDK patch needs LF line endings. See Copy-SymlinkTargets.
$env:GIT_CONFIG_COUNT = '2'
$env:GIT_CONFIG_KEY_0 = 'core.autocrlf'
$env:GIT_CONFIG_VALUE_0 = 'false'
$env:GIT_CONFIG_KEY_1 = 'core.symlinks'
$env:GIT_CONFIG_VALUE_1 = 'false'

Install-Game
Install-Translation
Build-Sdk

$env:REXSDK = Join-Path $Tools 'rexsdk'
if (-not (Test-Path (Join-Path $Root 'generated\default\sources.cmake'))) {
  Step 'Recompiling the game code'
  Push-Location $Root
  Run (Join-Path $env:REXSDK 'bin\rexglue.exe') codegen (Join-Path $Root 'force_manifest.toml')
  Pop-Location
}

Step 'Building the game'
Run (Join-Path $Root 'build.bat') release
if (-not (Test-Path $Exe)) { Fail 'The build finished but force.exe is missing.' }

Step ("Done in {0:N0} minutes." -f ((Get-Date) - $started).TotalMinutes)
Stop-Transcript | Out-Null
