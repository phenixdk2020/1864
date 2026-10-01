# Spejler slagdelen fra Strategy1864 (Unity-portens Unreal-projekt) ind i kampagneprojektet (Game1864).
# Strategy1864 er stadig stedet, hvor slaget udvikles; dette script kopierer det over (Docs/BattleSync1864.md).
#
#   Kildekoden (modulet Strategy1864) tages fra den seneste commit i Strategy1864 (det stabile), eller med
#   -WorkingTree fra arbejdsmappen, som den står lige nu (også det, der ikke er committet endnu).
#   Indholdet (kort, soldater, animationer under /Game/Units og kortene Strategy1864_*) tages fra arbejdsmappen.
#   Tastaturbindingerne flettes ind i Config/DefaultInput.ini mellem to markeringer.
#
# Brug:  powershell -File Tools\Battle\Sync-Battle.ps1 [-WorkingTree] [-Source <sti>]

param(
    [string]$Source = "R:\Onedrive\Dokumenter\Unreal Projects\Strategy1864",
    [switch]$WorkingTree
)

$ErrorActionPreference = "Stop"
$Target = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$SrcUnreal = Join-Path $Source "Unreal"
if (-not (Test-Path (Join-Path $SrcUnreal "Strategy1864.uproject"))) { throw "Fandt ikke Strategy1864.uproject under $SrcUnreal" }
$Git = @("-c", "safe.directory=*", "-C", $Source)

# ---- kildekoden
$Commit = (& git @Git rev-parse --short HEAD).Trim()
$ModuleTarget = Join-Path $Target "Source\Strategy1864"
if ($WorkingTree) {
    $ModuleSource = Join-Path $SrcUnreal "Source\Strategy1864"
    $From = "arbejdsmappen (oven på $Commit)"
} else {
    $Temp = Join-Path $env:TEMP ("Strategy1864Sync_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force $Temp | Out-Null
    $Tar = Join-Path $Temp "src.tar"
    & git @Git archive --format=tar -o $Tar HEAD "Unreal/Source/Strategy1864"
    & (Join-Path $env:SystemRoot "System32\tar.exe") -xf $Tar -C $Temp   # Windows' own tar (Git Bash' tar takes C: for a host)
    $ModuleSource = Join-Path $Temp "Unreal\Source\Strategy1864"
    $From = "commit $Commit"
}
robocopy $ModuleSource $ModuleTarget /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy fejlede for kildekoden ($LASTEXITCODE)" }
if (-not $WorkingTree) { Remove-Item -Recurse -Force $Temp }
# The copies get the time of copying: robocopy keeps the source times, and an older time makes the build
# reuse stale object files.
Get-ChildItem $ModuleTarget -Recurse -File | ForEach-Object { $_.LastWriteTime = Get-Date }

# ---- indholdet: /Game/Units (spejlet) og slagkortene Strategy1864_* (kun kopieret, kampagnens kort røres ikke)
robocopy (Join-Path $SrcUnreal "Content\Units") (Join-Path $Target "Content\Units") /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy fejlede for Content\Units ($LASTEXITCODE)" }
$Maps = Join-Path $Target "Content\Maps"
New-Item -ItemType Directory -Force $Maps | Out-Null
Get-ChildItem (Join-Path $SrcUnreal "Content\Maps") -Filter "Strategy1864_*" | ForEach-Object { Copy-Item $_.FullName $Maps -Force }

# ---- tastaturbindingerne
$Begin = "; ---- Strategy1864 (slaget): skrevet af Tools/Battle/Sync-Battle.ps1, ret dem i Strategy1864 ----"
$End = "; ---- Strategy1864 slut ----"
$Their = Get-Content (Join-Path $SrcUnreal "Config\DefaultInput.ini") | Where-Object { $_ -match '^\+(ActionMappings|AxisMappings)=' }
$InputIni = Join-Path $Target "Config\DefaultInput.ini"
$Ours = [IO.File]::ReadAllText($InputIni)
$Block = $Begin + "`r`n" + ($Their -join "`r`n") + "`r`n" + $End
if ($Ours.Contains($Begin)) {
    $Start = $Ours.IndexOf($Begin)
    $Stop = $Ours.IndexOf($End, $Start) + $End.Length
    $Ours = $Ours.Substring(0, $Start) + $Block + $Ours.Substring($Stop)
} else {
    $Ours = $Ours.TrimEnd() + "`r`n" + $Block + "`r`n"
}
[IO.File]::WriteAllText($InputIni, $Ours, (New-Object Text.UTF8Encoding $false))

# ---- notér hvad der blev spejlet
$Note = "Strategy1864 spejlet {0} fra {1}  ·  {2} bindinger" -f (Get-Date -Format "yyyy-MM-dd HH:mm"), $From, $Their.Count
[IO.File]::WriteAllText((Join-Path $PSScriptRoot "LastSync.txt"), $Note + "`r`n", (New-Object Text.UTF8Encoding $false))
Write-Host $Note
