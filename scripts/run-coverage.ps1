# Generate NetPulse test coverage report (Phase 0 baseline).
# Usage:
#   .\scripts\run-coverage.ps1 -Backend llvm
#   .\scripts\run-coverage.ps1 -Backend opencpp -Config Debug

param(
    [ValidateSet("llvm", "opencpp")]
    [string]$Backend = "opencpp",
    [string]$BuildDir = "build-coverage",
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

function Invoke-LlvmCoverage {
    cmake -S . -B $BuildDir -G "MinGW Makefiles" -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON
    cmake --build $BuildDir --target NetPulseTests -j 4

    $TestExe = Join-Path $BuildDir "tests\NetPulseTests.exe"
    $ProfRawDir = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot (Join-Path $BuildDir "coverage-profraw")))
    $ProfData = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot (Join-Path $BuildDir "coverage.profdata")))
    $ReportDir = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot (Join-Path $BuildDir "coverage-report")))

    if (Test-Path $ProfRawDir) {
        Remove-Item $ProfRawDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $ProfRawDir | Out-Null
    if (Test-Path $ReportDir) {
        Remove-Item $ReportDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $ReportDir | Out-Null

    $env:LLVM_PROFILE_FILE = Join-Path $ProfRawDir "NetPulseTests-%p.profraw"
    $AbsoluteTestExe = (Resolve-Path $TestExe).Path
    Push-Location (Split-Path $AbsoluteTestExe)
    & $AbsoluteTestExe
    if ($LASTEXITCODE -ne 0) {
        throw "NetPulseTests failed with exit code $LASTEXITCODE"
    }
    Pop-Location

    llvm-profdata merge -sparse (Join-Path $ProfRawDir "*.profraw") -o $ProfData

    $IgnoreRegex = "third_party|tests\\"
    llvm-cov export $TestExe -instr-profile $ProfData `
        -format=lcov `
        -ignore-filename-regex=$IgnoreRegex `
        | Out-File -Encoding ascii (Join-Path $ReportDir "coverage.lcov")

    llvm-cov report $TestExe -instr-profile $ProfData `
        -ignore-filename-regex=$IgnoreRegex `
        | Tee-Object -FilePath (Join-Path $ReportDir "summary.txt")

    Write-Host "LLVM coverage report: $ReportDir"
}

function Invoke-OpenCppCoverage {
    cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
    cmake --build $BuildDir --config $Config --target NetPulseTests

    $TestExe = Join-Path $BuildDir "$Config\tests\NetPulseTests.exe"
    if (-not (Test-Path $TestExe)) {
        throw "Test binary not found: $TestExe"
    }

    $HtmlDir = Join-Path $BuildDir "coverage-html"
    $Cobertura = Join-Path $BuildDir "coverage.xml"

    if (-not (Get-Command OpenCppCoverage -ErrorAction SilentlyContinue)) {
        throw "OpenCppCoverage not found. Install from https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
    }

    OpenCppCoverage `
        --sources "$RepoRoot\src" `
        --sources "$RepoRoot\include" `
        --excluded_sources "$RepoRoot\third_party" `
        --excluded_sources "$RepoRoot\tests" `
        --export_type "cobertura:$Cobertura" `
        --export_type "html:$HtmlDir" `
        -- $TestExe

    if ($LASTEXITCODE -ne 0) {
        throw "OpenCppCoverage failed with exit code $LASTEXITCODE"
    }

    Write-Host "Cobertura XML: $Cobertura"
    Write-Host "HTML report: $HtmlDir"
}

switch ($Backend) {
    "llvm" { Invoke-LlvmCoverage }
    "opencpp" { Invoke-OpenCppCoverage }
}
