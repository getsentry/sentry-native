# WER does not store reports while diagnostic data is off
$DataCollectionKeys = @(
    'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\DataCollection'
    'HKLM:\SOFTWARE\Policies\Microsoft\Windows\DataCollection'
    'HKLM:\SOFTWARE\Wow6432Node\Policies\Microsoft\Windows\DataCollection'
)
foreach ($Key in $DataCollectionKeys) {
    if (Test-Path $Key) {
        Set-ItemProperty $Key -Name AllowTelemetry -Value 1 -Type DWord
    }
}

# a crash dialog must not hang the job
$WerKey = 'HKCU:\Software\Microsoft\Windows\Windows Error Reporting'
if (!(Test-Path $WerKey)) {
    New-Item $WerKey | Out-Null
}
Set-ItemProperty $WerKey -Name DontShowUI -Value 1 -Type DWord
