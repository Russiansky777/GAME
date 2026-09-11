param([string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'LowTide.uproject'
$reportDirectory = Join-Path $projectRoot 'Artifacts/Tests'
$started = Get-Date
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -unattended -NullRHI -nosound '-ExecCmds=Automation RunTests LowTide.M05' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$reportDirectory" -log
if ($LASTEXITCODE -ne 0) { throw "Unreal tests exited with $LASTEXITCODE" }
$reportFile = Get-Item (Join-Path $reportDirectory 'index.json')
if ($reportFile.LastWriteTime -lt $started) { throw 'Test report is stale' }
$report = Get-Content $reportFile.FullName -Raw | ConvertFrom-Json
$report.tests | Select-Object fullTestPath, state | Format-Table -AutoSize
if ($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0 -or ($report.succeeded + $report.succeededWithWarnings) -lt 3) {
    throw 'LOW TIDE automation did not pass all required tests'
}
