Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

Write-Host "=== 1. GIT REMOTES ===" -ForegroundColor Cyan
git remote -v
Write-Host ""
Write-Host "=== 2. GIT SUBMODULES ===" -ForegroundColor Cyan
git submodule status 2>&1
Write-Host ""
if (Test-Path '.gitmodules') {
    Get-Content '.gitmodules'
} else {
    Write-Host "  No .gitmodules file"
}

Write-Host ""
Write-Host "=== 3. ALL GIT REFS ===" -ForegroundColor Cyan
git for-each-ref --format='%(refname) %(objectname:short) %(subject)' 2>&1

Write-Host ""
Write-Host "=== 4. GIT REFLOG (HEAD) ===" -ForegroundColor Cyan
git reflog HEAD --format='%h %gd %gs' 2>&1

Write-Host ""
Write-Host "=== 5. GIT REFLOG (ALL) ===" -ForegroundColor Cyan
git reflog --all --format='%h %gd %gs' 2>&1

Write-Host ""
Write-Host "=== 6. SEARCH FOR 'private' OR 'planning' IN ALL FILES ===" -ForegroundColor Cyan
$privateHits = Get-ChildItem . -Recurse -File -Include '*.md','*.txt','*.json','*.yaml','*.yml','*.cfg','*.conf','*.sh','*.ps1' -ErrorAction SilentlyContinue |
    Select-String -Pattern 'private|planning.repository|authority.repository' -CaseSensitive:$false -ErrorAction SilentlyContinue
if ($privateHits) {
    foreach ($hit in $privateHits) {
        Write-Host "  $($hit.Path):$($hit.LineNumber): $($hit.Line.Trim())"
    }
} else {
    Write-Host "  No matches"
}

Write-Host ""
Write-Host "=== 7. GIT LOG ALL ===" -ForegroundColor Cyan
git log --all --oneline --decorate

Write-Host ""
Write-Host "=== 8. GIT TAGS ===" -ForegroundColor Cyan
git tag -l

Write-Host ""
Write-Host "=== 9. UNREACHABLE OBJECTS ===" -ForegroundColor Cyan
git fsck --unreachable --no-reflogs 2>&1 | Select-Object -First 30
