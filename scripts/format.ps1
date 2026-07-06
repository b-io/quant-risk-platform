param(
    [switch]$Check,
    [switch]$Fix,
    [ValidateSet("all", "cpp", "python")]
    [string]$Scope = "all",
    [string[]]$Path = @()
)

# Format or check C++ and Python source files.

$scriptPath = Join-Path $PSScriptRoot "format.py"
$arguments = @("--scope", $Scope)

if ($Fix) {
    $arguments += "--fix"
} elseif ($Check) {
    $arguments += "--check"
}

$arguments += $Path

python $scriptPath @arguments
exit $LASTEXITCODE
