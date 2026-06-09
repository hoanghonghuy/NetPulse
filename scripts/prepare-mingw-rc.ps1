# Prepares app.rc for llvm-mingw windres.
# windres treats UTF-8 literals as Latin-1 unless the string uses the L"" prefix
# and the compiler is invoked with --codepage=65001.

param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-ContainsNonAscii {
    param([string]$Value)

    foreach ($ch in $Value.ToCharArray()) {
        if ([int][char]$ch -ge 0x80) {
            return $true
        }
    }

    return $false
}

function Convert-RcQuotedStrings {
    param([string]$Line)

    $regex = [regex]::new('L?"(?:[^"\\]|\\.)*"')
    $matches = $regex.Matches($Line)
    if ($matches.Count -eq 0) {
        return $Line
    }

    $result = $Line
    for ($index = $matches.Count - 1; $index -ge 0; $index--) {
        $match = $matches[$index]
        if ($match.Value.StartsWith('L"')) {
            continue
        }

        $inner = $match.Value.Substring(1, $match.Value.Length - 2)
        if (-not (Test-ContainsNonAscii $inner)) {
            continue
        }

        $replacement = 'L"' + $inner + '"'
        $result = $result.Substring(0, $match.Index) + $replacement + $result.Substring($match.Index + $match.Length)
    }

    return $result
}

$utf8WithBom = New-Object System.Text.UTF8Encoding $true
$lines = [System.IO.File]::ReadAllLines($InputPath, $utf8WithBom)
$outputLines = foreach ($line in $lines) {
    Convert-RcQuotedStrings $line
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory -and -not (Test-Path $outputDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

[System.IO.File]::WriteAllLines($OutputPath, $outputLines, $utf8WithBom)
