param (
    [switch]$Unit = $false,
    [switch]$Clean = $false,
    [string]$Keyword = "",
    [int]$Parallelism = 1,
    [switch]$WithoutCrashpadWer = $false,
    [switch]$DisableCapture = $false
)

$update_test_discovery = Join-Path -Path $PSScriptRoot -ChildPath "update_test_discovery.ps1"
& $update_test_discovery

if ($Clean -or -not (Test-Path .\.venv))
{
    Remove-Item -Recurse -Force .\.venv\ -ErrorAction SilentlyContinue
    python3.exe -m venv .venv
    .\.venv\Scripts\pip.exe install --upgrade --requirement .\tests\requirements.txt
}

$pytestCommand = ".\.venv\Scripts\pytest.exe .\tests\ --verbose"

if ($DisableCapture) {
    $pytestCommand += " --capture=no"
}

if ($Parallelism -gt 1)
{
    $pytestCommand += " -n $Parallelism"
}

if (-not $WithoutCrashpadWer -and -not $Unit)
{
    $pytestCommand += " --with_crashpad_wer"
}

if ($Keyword)
{
    $pytestCommand += " -k `"$Keyword`""
}
elseif ($Unit)
{
    $pytestCommand += " -k `"unit`""
}

Invoke-Expression $pytestCommand