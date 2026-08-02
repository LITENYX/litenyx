Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

Write-Host "=== 1. TEXTUAL H_MSF REFERENCES ===" -ForegroundColor Cyan
Write-Host "Searching git grep..."
$grepResult = git grep -n -i $H_MSF 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "FOUND:" -ForegroundColor Green
    Write-Host $grepResult
} else {
    Write-Host "NOT FOUND in tracked files" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Searching git log -S..."
$logResult = git log --all -S $H_MSF --oneline --decorate --stat 2>&1
if ($logResult -match '\w{7,40}') {
    Write-Host "FOUND:" -ForegroundColor Green
    Write-Host $logResult
} else {
    Write-Host "NOT FOUND in commit history" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== 2. MSF / CONTRACT CANDIDATES (tracked) ===" -ForegroundColor Cyan
$trackedCandidates = git ls-files 2>&1 |
    Select-String -Pattern 'MSF|contract|experiment|scenario|scientific' -CaseSensitive:$false
if ($trackedCandidates) {
    foreach ($line in $trackedCandidates) {
        Write-Host "  $line"
    }
} else {
    Write-Host "  No tracked file matches" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== 3. FILESYSTEM CANDIDATES (all, including untracked) ===" -ForegroundColor Cyan
$fsCandidates = Get-ChildItem . -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match 'MSF|contract|experiment|scenario|scientific' }
if ($fsCandidates) {
    foreach ($f in $fsCandidates) {
        Write-Host "  $($f.FullName)  [$($f.Length) bytes, $($f.LastWriteTimeUtc)]"
    }
} else {
    Write-Host "  No filesystem matches" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== 4. BROAD CONTENT SEARCH ===" -ForegroundColor Cyan
Write-Host "Searching all text files for 'MSF' keyword..."
$msfContentHits = Get-ChildItem . -Recurse -File -Include '*.md','*.txt','*.json','*.yaml','*.yml','*.cfg','*.conf','*.sh','*.ps1' -ErrorAction SilentlyContinue |
    Select-String -Pattern 'MSF' -CaseSensitive:$false -ErrorAction SilentlyContinue
if ($msfContentHits) {
    foreach ($hit in $msfContentHits) {
        Write-Host "  $($hit.Path):$($hit.LineNumber): $($hit.Line.Trim())"
    }
} else {
    Write-Host "  No content matches for MSF" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== 5. SEARCH FOR 'fa003d0f' (short prefix) ===" -ForegroundColor Cyan
$shortHash = 'fa003d0f'
$shortHits = Get-ChildItem . -Recurse -File -Include '*.md','*.txt','*.json','*.yaml','*.yml','*.ps1','*.sh','*.cfg','*.conf' -ErrorAction SilentlyContinue |
    Select-String -Pattern $shortHash -CaseSensitive:$false -ErrorAction SilentlyContinue
if ($shortHits) {
    foreach ($hit in $shortHits) {
        Write-Host "  $($hit.Path):$($hit.LineNumber): $($hit.Line.Trim())"
    }
} else {
    Write-Host "  No content matches for fa003d0f" -ForegroundColor Yellow
}
