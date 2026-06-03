# Builds Capital Vice Release, stages files, and creates a Windows installer when Inno Setup is available.
# Usage:  powershell -ExecutionPolicy Bypass -File scripts\build_installer.ps1

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build"
$StageDir = Join-Path $RepoRoot "dist\CapitalVice"
$DistDir = Join-Path $RepoRoot "dist"
$IssFile = Join-Path $RepoRoot "installer\CapitalVice.iss"

Write-Host "=== Capital Vice installer build ===" -ForegroundColor Cyan

if (-not (Test-Path $BuildDir)) {
    Write-Host "Configuring CMake..."
    cmake -S $RepoRoot -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DCAPITALVICE_DEV_CONSOLE=OFF
}

Write-Host "Building Release..."
cmake --build $BuildDir --config Release --target capital_vice
$ExePath = Join-Path $BuildDir "Release\capital_vice.exe"
if (-not (Test-Path $ExePath)) {
    throw "Build failed: $ExePath not found"
}

Write-Host "Staging to $StageDir ..."
if (Test-Path $StageDir) {
    Remove-Item $StageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

Copy-Item $ExePath (Join-Path $StageDir "capital_vice.exe")
Copy-Item (Join-Path $RepoRoot "README.md") (Join-Path $StageDir "README.md")
Copy-Item (Join-Path $RepoRoot "installer\readme_after_install.txt") (Join-Path $StageDir "PLAYING.txt")

$PortableZip = Join-Path $DistDir "CapitalVice-Portable-0.1.0.zip"
if (Test-Path $PortableZip) {
    Remove-Item $PortableZip -Force
}
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $PortableZip
Write-Host "Portable zip: $PortableZip" -ForegroundColor Green

$IsccCandidates = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
    "ISCC.exe"
)
$Iscc = $null
foreach ($candidate in $IsccCandidates) {
    if ($candidate -eq "ISCC.exe") {
        $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
        if ($cmd) { $Iscc = $cmd.Source; break }
    } elseif (Test-Path $candidate) {
        $Iscc = $candidate
        break
    }
}

if ($Iscc) {
    Write-Host "Running Inno Setup: $Iscc"
    & $Iscc $IssFile
    $SetupExe = Join-Path $DistDir "CapitalVice-Setup-0.1.0.exe"
    if (Test-Path $SetupExe) {
        Write-Host "Installer: $SetupExe" -ForegroundColor Green
    } else {
        Write-Warning "Inno Setup finished but setup exe was not found in dist\"
    }
} else {
    Write-Host ""
    Write-Host "Inno Setup 6 not found. Install from https://jrsoftware.org/isinfo.php" -ForegroundColor Yellow
    Write-Host "Then re-run this script, or compile manually:" -ForegroundColor Yellow
    Write-Host "  `"${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe`" `"$IssFile`""
    Write-Host ""
    Write-Host "Portable zip is ready without Inno Setup." -ForegroundColor Yellow
}

Write-Host "Done."
