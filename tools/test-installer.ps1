$ErrorActionPreference = 'Stop'
$setup = (Resolve-Path 'dist/LANDELTA-Setup-0.13.4.exe').Path
$target = Join-Path $env:RUNNER_TEMP 'LANDELTA-install-test'
$start = [Environment]::GetFolderPath('Startup')
$startupLink = Join-Path $start 'LANDELTA-Auto.lnk'
$data = Join-Path $env:LOCALAPPDATA 'VortexACC'
New-Item -ItemType Directory -Force $data, $start | Out-Null
$sentinel = Join-Path $data 'installer-test-sentinel.txt'
Set-Content $sentinel 'preserve telemetry and settings'
$shell = New-Object -ComObject WScript.Shell
$link = $shell.CreateShortcut($startupLink)
$link.TargetPath = Join-Path $env:RUNNER_TEMP 'old-copy\LANDELTA.exe'
$link.Arguments = '--background'
$link.Save()
function Install {
    $p = Start-Process $setup -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/DIR=`"$target`"" -Wait -PassThru
    if ($p.ExitCode -ne 0) { throw "Installer failed: $($p.ExitCode)" }
}
Install
Install # Reinstall/upgrade to same folder must keep one app and preserve data.
$exe = Join-Path $target 'LANDELTA.exe'
if ((Get-Item $exe).VersionInfo.ProductVersion -ne '0.13.4') { throw 'Wrong installed version' }
if (!(Test-Path (Join-Path $target 'tools\ac-analyzer.html'))) { throw 'Analyzer missing' }
if (Get-ChildItem $target -Recurse -Include '*.bat','*.cpp','*.hpp','CAMBIOS*') { throw 'Development files leaked' }
foreach ($folder in @([Environment]::GetFolderPath('Programs'), [Environment]::GetFolderPath('Desktop'))) {
    $shortcut = $shell.CreateShortcut((Join-Path $folder 'LANDELTA.lnk'))
    if ($shortcut.TargetPath -ne $exe) { throw 'Wrong user shortcut target' }
}
$link = $shell.CreateShortcut($startupLink)
if ($link.TargetPath -ne $exe -or $link.Arguments -ne '--background') { throw 'Startup migration failed' }
$appMutex = [System.Threading.Mutex]::new($false, 'Local\VortexACCOverlay')
try {
    $blocked = Start-Process $setup -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=`"$target`"" -Wait -PassThru
    if ($blocked.ExitCode -eq 0) { throw 'Installer ignored running application' }
} finally { $appMutex.Dispose() }
$uninstaller = Join-Path $target 'unins000.exe'
$p = Start-Process $uninstaller -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Wait -PassThru
if ($p.ExitCode -ne 0) { throw 'Uninstall failed' }
if (!(Test-Path $sentinel)) { throw 'User data was deleted' }
if (Test-Path $exe) { throw 'Executable remained after uninstall' }
Write-Output 'PASS: Windows install, reinstall, version, runtime files, shortcuts, startup migration, mutex guard and data-preserving uninstall'
