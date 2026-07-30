Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

Write-Host "=== CANDIDATE FILE HASHING ===" -ForegroundColor Cyan

$candidates = Get-ChildItem . -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match 'MSF|contract|experiment|scenario|scientific' -and $_.FullName -notmatch '\\\.git\\' }

foreach ($f in $candidates) {
    $h = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
    $match = $h -eq $H_MSF
    $mark = if ($match) { 'MATCH' } else { '' }
    $color = if ($match) { 'Green' } else { 'DarkGray' }
    Write-Host "  $h  $($f.Name)  $mark" -ForegroundColor $color
}

Write-Host ""
Write-Host "=== DOGECOIN EXPERIMENTS.MD CONTENT ===" -ForegroundColor Cyan
Get-Content 'deploy/external/dogecoin/doc/experiments.md' -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== SEARCH ORCH/HARNESS DIRECTIVE HISTORY ===" -ForegroundColor Cyan
Write-Host "Searching all commits for 'ORCH'..."
$orchLog = git log --all --oneline --grep='ORCH' 2>&1
Write-Host $orchLog

Write-Host ""
Write-Host "Searching all commits for 'harness'..."
$harnessLog = git log --all --oneline --grep='harness' -i 2>&1
Write-Host $harnessLog

Write-Host ""
Write-Host "Searching all commits for 'H_MSF'..."
$msfLog = git log --all --oneline --grep='H_MSF' 2>&1
Write-Host $msfLog

Write-Host ""
Write-Host "Searching all commits for 'frozen'..."
$frozenLog = git log --all --oneline --grep='frozen' -i 2>&1
Write-Host $frozenLog

Write-Host ""
Write-Host "=== GIT REFLOG (all refs) ===" -ForegroundColor Cyan
$reflog = git reflog --all 2>&1
Write-Host $reflog

Write-Host ""
Write-Host "=== SEARCH FOR NONCE/SCENARIO/PARAMETER FILES ===" -ForegroundColor Cyan
$nonceCandidates = Get-ChildItem . -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match 'nonce|parameter|config|manifest|schema' -and $_.Extension -match '\.(json|yaml|yml|toml|cfg|conf|txt|md)$' -and $_.FullName -notmatch '\\\.git\\' -and $_.FullName -notmatch 'node_modules' }
foreach ($f in $nonceCandidates) {
    Write-Host "  $($f.FullName)  [$($f.Length) bytes]"
}

Write-Host ""
Write-Host "=== LITENYX ROOT-LEVEL FILES ===" -ForegroundColor Cyan
Get-ChildItem . -File | Select-Object Name,Length,LastWriteTimeUtc | Format-Table -AutoSize
