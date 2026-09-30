# Build and run the reconstructed gameplay code (everything stays inside gameplay/).
# Usage (from anywhere):  powershell -ExecutionPolicy Bypass -File gameplay\build_and_run.ps1
# Runs: CMake configure + Release build into gameplay\build, all CTest suites, then the stage simulator.
$ErrorActionPreference = 'Stop'
$here  = Split-Path -Parent $MyInvocation.MyCommand.Path
$src   = Join-Path $here 'src'
$build = Join-Path $here 'build'
$files = if ($env:SHADOW_GAME_FILES_DIR) {
    $env:SHADOW_GAME_FILES_DIR
} else {
    Join-Path (Split-Path -Parent $here) 'files'
}
if (-not (Test-Path -LiteralPath (Join-Path $files 'nukkoro2.inf') -PathType Leaf)) {
    throw "game files not found; set SHADOW_GAME_FILES_DIR to the read-only extracted files directory"
}

cmake -S $src -B $build "-DSHADOW_GAME_FILES_DIR=$files" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "configure failed" }
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw "build failed" }
ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }
& (Join-Path $build 'Release\stage_sim.exe') $files 100
if ($LASTEXITCODE -ne 0) { throw "stage_sim failed" }
Write-Host "build_and_run: OK"
