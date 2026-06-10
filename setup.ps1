$taskName = "DisableKeyboardBacklight_$([System.Environment]::UserName)"
$program = "ideapad_keyboard_backlight"

$userid = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$action = New-ScheduledTaskAction -Execute "$((Get-Location).Path)\$($program).exe" -Argument "--backlight 0 --charge 2 --its 1"
$trigger = New-ScheduledTaskTrigger -AtStartup
# wait for required services to start
$trigger.Delay = "PT5S"
# use current userid, since we read from HKCU
$principal = New-ScheduledTaskPrincipal -UserId $userid -LogonType S4U
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal -Settings $settings
Write-Output "Scheduled task '$taskName' has been created successfully."
