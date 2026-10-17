# Builds Artilody Sampleforge from source on Windows 10/11 (x64).
# Requirements: Visual Studio 2022 or "Build Tools for Visual Studio 2022" with
# the "Desktop development with C++" workload (includes CMake).
# Optional: Inno Setup 6 to also create the installer.
# Run by double-clicking Build-Windows.bat.
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vs) {
            $c = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $c) { return $c }
        }
    }
    return $null
}

$cmake = Find-CMake
if (-not $cmake) {
    Write-Host "CMake / Visual Studio 2022 C++ tools not found." -ForegroundColor Red
    Write-Host "Install 'Build Tools for Visual Studio 2022' with 'Desktop development with C++':"
    Write-Host "  https://visualstudio.microsoft.com/downloads/"
    exit 1
}

$build = Join-Path $PSScriptRoot "build-windows"
Write-Host "Building Artilody Sampleforge (Release) - first build downloads JUCE 8.0.4."
& $cmake -S . -B $build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
if ($LASTEXITCODE) { throw "CMake configure failed" }
& $cmake --build $build --config Release --parallel
if ($LASTEXITCODE) { throw "Build failed" }
$ctest = Join-Path (Split-Path $cmake) "ctest.exe"
& $ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE) { throw "Self-test failed" }

$exe = Join-Path $build "Sampleforge_artefacts\Release\Artilody Sampleforge.exe"
if (-not (Test-Path $exe)) { throw "Executable not found: $exe" }
Write-Host "Built: $exe" -ForegroundColor Green

$iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
          "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($iscc) {
    $version = (Select-String -Path CMakeLists.txt -Pattern 'project\(ArtilodySampleforge VERSION ([0-9.]+)').Matches[0].Groups[1].Value
    & $iscc "/DAppVersion=$version" "/DBuildDir=$build" "packaging\windows\installer.iss"
    if ($LASTEXITCODE) { throw "Installer build failed" }
    Write-Host "Installer created in dist\" -ForegroundColor Green
} else {
    Write-Host "Inno Setup 6 not found - skipping installer. The program can be run directly from the path above."
}
Start-Process $exe
