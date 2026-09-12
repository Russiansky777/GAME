param([string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'LowTide.uproject'
$reportDirectory = Join-Path $projectRoot 'Artifacts/Tests'
$started = Get-Date
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -unattended -NullRHI -nosound '-ExecCmds=Automation RunTests LowTide' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$reportDirectory" -log
if ($LASTEXITCODE -ne 0) { throw "Unreal tests exited with $LASTEXITCODE" }
$reportFile = Get-Item (Join-Path $reportDirectory 'index.json')
if ($reportFile.LastWriteTime -lt $started) { throw 'Test report is stale' }
$report = Get-Content $reportFile.FullName -Raw | ConvertFrom-Json
$report.tests | Select-Object fullTestPath, state | Format-Table -AutoSize
$requiredTests = @(
    'LowTide.M05.Expedition.RoundTrip',
    'LowTide.M05.Expedition.SafeEdgeReturn',
    'LowTide.M05.Traversal.ControlsAndGrounding',
    'LowTide.M05.Inventory.AtomicIndividualSale',
    'LowTide.M05.Inventory.CapacityAndQuantityBoundaries',
    'LowTide.M1.Mission.UniqueRewardAndRareChoice',
    'LowTide.M1.Expedition.LivingTideAndAlternateRoute',
    'LowTide.M1.Risk.InvalidGroundRecovery',
    'LowTide.M1.Traversal.JumpContainment',
    'LowTide.M1.Risk.PhenomenonRecoveryAndSecondTrip',
    'LowTide.M1.Scene.Containment',
    'LowTide.M1.Hub.LivelinessContract'
)
$passedTests = @($report.tests | Where-Object { $_.state -eq 'Success' } | ForEach-Object fullTestPath)
if ($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0 -or ($report.succeeded + $report.succeededWithWarnings) -lt $requiredTests.Count -or @($requiredTests | Where-Object { $_ -notin $passedTests }).Count -ne 0) {
    throw 'LOW TIDE automation did not pass all required tests'
}
