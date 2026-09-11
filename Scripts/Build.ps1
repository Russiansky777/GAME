param(
    [ValidateSet('Editor', 'Package')][string]$Mode = 'Editor',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'LowTide.uproject'
if (!(Test-Path "$EngineRoot\Engine\Build\Build.version")) { throw "Engine not found: $EngineRoot" }
if ($Mode -eq 'Editor') {
    & "$EngineRoot\Engine\Build\BatchFiles\Build.bat" LowTideEditor Win64 Development "-Project=$project" -WaitMutex -MaxParallelActions=2
} else {
    & "$EngineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$projectRoot\Artifacts\Windows" -prereqs -utf8output '-ubtargs=-MaxParallelActions=2'
}
if ($LASTEXITCODE -ne 0) { throw "Unreal $Mode failed with exit code $LASTEXITCODE" }
if ($Mode -eq 'Package') {
    $sourceCatalog = Join-Path $projectRoot 'Content/Data/Items.json'
    $stagedCatalog = Join-Path $projectRoot 'Artifacts/Windows/LowTide/Content/Data/Items.json'
    if (!(Test-Path $stagedCatalog) -or (Get-FileHash $sourceCatalog).Hash -ne (Get-FileHash $stagedCatalog).Hash) {
        throw 'Packaged item catalog is missing or differs from source'
    }
}
