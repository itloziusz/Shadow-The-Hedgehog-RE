# Build and run the reconstructed gameplay code (everything stays inside gameplay/).
# Usage (from anywhere):  powershell -ExecutionPolicy Bypass -File gameplay\build_and_run.ps1
# Runs: CMake configure + Release build into gameplay\build, all CTest suites, then the stage simulator.
$ErrorActionPreference = 'Stop'
$here  = Split-Path -Parent $MyInvocation.MyCommand.Path
$src   = Join-Path $here 'src'
$build = Join-Path $here 'build'
$files = Join-Path (Split-Path -Parent $here) 'files'   # game data, read-only

cmake -S $src -B $build | Out-Null
cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw "build failed" }
ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }
& (Join-Path $build 'Release\stage_sim.exe') $files 100
if ($LASTEXITCODE -ne 0) { throw "stage_sim failed" }
Write-Host "build_and_run: OK"
