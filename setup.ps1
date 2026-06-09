$taskName = "Disable Keyboard Backlight"
$program = "ideapad_keyboard_backlight"

$action = New-ScheduledTaskAction -Execute "$((Get-Location).Path)\$($program).exe" -Argument "--backlight 0 --charge 2 --its 1"
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings
Write-Output "Scheduled task '$taskName' has been created successfully."
