Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

Write-Host "=== 1. SEARCH PHASE-GREEN TAGS FOR MSF/CONTRACT/HASH ===" -ForegroundColor Cyan
$tags = @('phase2-green','phase3-green','phase4-green','phase5-green','phase6-green','pre-integration-baseline')
foreach ($tag in $tags) {
    $result = git log $tag --oneline --grep='MSF\|contract\|frozen.*spec\|ORCH\|H_MSF' -i 2>&1
    if ($result -match '\w{7,40}') {
        Write-Host "  TAG $tag MATCHES:"
        Write-Host "  $result"
    }
}

Write-Host ""
Write-Host "=== 2. SEARCH UNREACHABLE OBJECTS ===" -ForegroundColor Cyan
$unreachable = git fsck --unreachable --no-reflogs 2>&1 | Select-String 'commit|blob|tree'
$commitHashes = @()
foreach ($line in $unreachable) {
    if ($line -match 'unreachable commit (\w+)') {
        $commitHashes += $Matches[1]
    }
}
Write-Host "  Found $($commitHashes.Count) unreachable commits"
foreach ($hash in $commitHashes) {
    $logResult = git log $hash --oneline -1 2>&1
    $bodyResult = git log $hash -1 --format='%B' 2>&1
    $hasMSF = $bodyResult -match 'MSF|H_MSF|fa003d|contract.*freeze|ORCH'
    $mark = if ($hasMSF) { 'MATCH' } else { '' }
    $color = if ($hasMSF) { 'Green' } else { 'DarkGray' }
    Write-Host "  $hash : $logResult  $mark" -ForegroundColor $color
    if ($hasMSF) {
        Write-Host "    BODY: $($bodyResult.Substring(0, [Math]::Min(200, $bodyResult.Length)))"
    }
}

Write-Host ""
Write-Host "=== 3. SEARCH ALL COMMITS FOR 'MSF' (full history) ===" -ForegroundColor Cyan
$msfCommits = git log --all --oneline --grep='MSF' -i 2>&1
Write-Host $msfCommits

Write-Host ""
Write-Host "=== 4. SEARCH ALL COMMITS FOR 'scientific' ===" -ForegroundColor Cyan
$sciCommits = git log --all --oneline --grep='scientific' -i 2>&1
Write-Host $sciCommits

Write-Host ""
Write-Host "=== 5. SEARCH ALL COMMITS FOR 'freeze.*contract|contract.*freeze' ===" -ForegroundColor Cyan
$freezeCommits = git log --all --oneline --grep='freeze.*contract\|contract.*freeze' -i 2>&1
Write-Host $freezeCommits

Write-Host ""
Write-Host "=== 6. SEARCH ALL COMMITS FOR 'spec.*frozen\|frozen.*spec' ===" -ForegroundColor Cyan
$frozenSpec = git log --all --oneline --grep='spec.*frozen\|frozen.*spec' -i 2>&1
Write-Host $frozenSpec

Write-Host ""
Write-Host "=== 7. SEARCH ALL COMMITS FOR 'canonical.*hash\|hash.*canonical' ===" -ForegroundColor Cyan
$canonHash = git log --all --oneline --grep='canonical.*hash\|hash.*canonical' -i 2>&1
Write-Host $canonHash

Write-Host ""
Write-Host "=== 8. LIST ALL DOCS/ SPEC FILES ===" -ForegroundColor Cyan
$specFiles = Get-ChildItem . -Recurse -File -Include '*.md' -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match 'spec|contract|authority|frozen' -and $_.FullName -notmatch '\\\.git\\' -and $_.FullName -notmatch 'deploy\\external' -and $_.FullName -notmatch 'external\\' }
foreach ($f in $specFiles) {
    $h = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
    $isMSF = $h -eq $H_MSF
    $mark = if ($isMSF) { 'MATCH' } else { '' }
    Write-Host "  $h  $($f.FullName)  $mark"
}

Write-Host ""
Write-Host "=== 9. SEARCH FOR ORCH-HARNESS-BUILD-1 ===" -ForegroundColor Cyan
$orchSearch = git log --all --oneline --grep='ORCH-HARNESS' -i 2>&1
Write-Host "  git log: $orchSearch"
$orchGrep = git grep -n 'ORCH-HARNESS' -- '*.md' '*.txt' '*.cpp' '*.h' '*.json' '*.yaml' '*.yml' 2>&1
Write-Host "  git grep: $orchGrep"
