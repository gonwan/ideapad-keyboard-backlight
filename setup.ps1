$taskName = "Disable Keyboard Backlight"
$program = "ideapad_keyboard_backlight"
$arch = "x64"
if ((Get-WmiObject Win32_operatingsystem).OSArchitecture -like "32*") {
    arch = "x86"
}
Set-Location -Path $PSScriptRoot
if (-not (Test-Path "$($program)-$($arch).exe")) {
    Expand-Archive -Path "$($program)_build.zip" -DestinationPath "."
}

$action = New-ScheduledTaskAction -Execute "$((Get-Location).Path)\$($program)-$($arch).exe" -Argument "0"
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings
Write-Output "Scheduled task '$taskName' has been created successfully."
