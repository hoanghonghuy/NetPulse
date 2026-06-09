param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [string]$Scenario = "launch-exit",
    [int]$TimeoutSec = 60
)

$ErrorActionPreference = "Stop"

$exe = Join-Path $BuildDir "NetPulse.exe"
if (-not (Test-Path $exe)) {
    Write-Error "NetPulse.exe not found at $exe. Build the project first."
}

$sandbox = Join-Path $env:TEMP ("NetPulseE2E_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $sandbox | Out-Null

$logPath = Join-Path $sandbox "e2e.log"
$errPath = Join-Path $sandbox "e2e.err"
$args = @(
    "--test-mode",
    "--test-scenario=$Scenario",
    "--sandbox-dir=$sandbox"
)

Write-Host "Running E2E scenario '$Scenario' with timeout ${TimeoutSec}s..."

$job = Start-Job -ScriptBlock {
    param($Executable, $ArgList, $Stdout, $Stderr)
    $proc = Start-Process -FilePath $Executable -ArgumentList $ArgList -PassThru -Wait `
        -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr
    return $proc.ExitCode
} -ArgumentList $exe, $args, $logPath, $errPath

$completed = Wait-Job $job -Timeout $TimeoutSec
if (-not $completed) {
    Stop-Job $job -Force
    Remove-Job $job -Force
    Get-Process -Name "NetPulse" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Write-Warning "E2E timed out; killed NetPulse process"
    if (Test-Path $logPath) { Write-Host "--- E2E stdout ---"; Get-Content $logPath }
    if (Test-Path $errPath) { Write-Host "--- E2E stderr ---"; Get-Content $errPath }
    exit 124
}

$exitCode = Receive-Job $job
Remove-Job $job -Force

if ($exitCode -ne 0) {
    Write-Warning "E2E failed with exit code $exitCode"
    if (Test-Path $logPath) { Write-Host "--- E2E stdout ---"; Get-Content $logPath }
    if (Test-Path $errPath) { Write-Host "--- E2E stderr ---"; Get-Content $errPath }
    exit $exitCode
}

Write-Host "E2E scenario '$Scenario' passed."
exit 0
