Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

Write-Host "=== 1. HASH ALL RECOVERY SPEC FILES ===" -ForegroundColor Cyan
$recoverySpecs = Get-ChildItem 'deploy/docs/recovery' -File -Filter '*.md' -ErrorAction SilentlyContinue
foreach ($f in $recoverySpecs) {
    $h = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
    $match = $h -eq $H_MSF
    $mark = if ($match) { 'MATCH' } else { '' }
    Write-Host "  $h  $($f.Name)  $mark"
}

Write-Host ""
Write-Host "=== 2. HASH ALL LITENYX HEADER/SPEC FILES ===" -ForegroundColor Cyan
$litSpecs = Get-ChildItem 'litenyx' -File -Filter '*.h' -ErrorAction SilentlyContinue
foreach ($f in $litSpecs) {
    $h = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
    $match = $h -eq $H_MSF
    $mark = if ($match) { 'MATCH' } else { '' }
    Write-Host "  $h  $($f.Name)  $mark"
}

Write-Host ""
Write-Host "=== 3. HASH ALL LITENYX DOCS ===" -ForegroundColor Cyan
$litDocs = Get-ChildItem 'docs' -File -Filter '*.md' -ErrorAction SilentlyContinue
foreach ($f in $litDocs) {
    $h = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash
    $match = $h -eq $H_MSF
    $mark = if ($match) { 'MATCH' } else { '' }
    Write-Host "  $h  $($f.Name)  $mark"
}

Write-Host ""
Write-Host "=== 4. SEARCH GIT OBJECTS FOR H_MSF PREFIX ===" -ForegroundColor Cyan
$blobResult = git log --all --pretty=format: --name-only --diff-filter=A 2>&1 | Sort-Object -Unique
Write-Host "  Total unique file paths in history: $($blobResult.Count)"

Write-Host ""
Write-Host "=== 5. CHECK IF 'MSF' APPEARS IN ANY HIDDEN/DELETED FILES ===" -ForegroundColor Cyan
$allDiffs = git log --all -p --diff-filter=D -- '*.md' '*.txt' '*.yaml' '*.yml' 2>&1 | Select-String -Pattern 'MSF|H_MSF|fa003d' -CaseSensitive:$false
if ($allDiffs) {
    Write-Host "  FOUND in deleted file diffs:"
    foreach ($line in $allDiffs) {
        Write-Host "    $($line.Line.Trim())"
    }
} else {
    Write-Host "  NOT FOUND in any deleted file diffs"
}

Write-Host ""
Write-Host "=== 6. CHECK ALL GIT BLOBS FOR H_MSF CONTENT ===" -ForegroundColor Cyan
Write-Host "  (Searching for fa003d0f in all blob objects...)"
$blobSearch = git log --all --pretty=format:'' --name-only 2>&1 | Sort-Object -Unique | Select-Object -First 500
$count = 0
foreach ($file in $blobSearch) {
    if ($file -and (Test-Path $file -ErrorAction SilentlyContinue)) {
        $content = Get-Content -LiteralPath $file -Raw -ErrorAction SilentlyContinue
        if ($content -and $content -match 'fa003d0f') {
            Write-Host "  FOUND in: $file" -ForegroundColor Green
        }
    }
    $count++
    if ($count -ge 200) { break }
}
Write-Host "  Searched $count files"

Write-Host ""
Write-Host "=== 7. CHECK FOR EXTERNAL GIT REPOS ===" -ForegroundColor Cyan
$gitDirs = Get-ChildItem 'C:\Users\sunilkr\New' -Directory -ErrorAction SilentlyContinue |
    Where-Object { Test-Path (Join-Path $_.FullName '.git') -ErrorAction SilentlyContinue }
foreach ($d in $gitDirs) {
    Write-Host "  Git repo: $($d.FullName)"
    $remote = git -C $d.FullName remote -v 2>&1
    Write-Host "    Remotes: $remote"
}

Write-Host ""
Write-Host "=== 8. CHECK FOR PRIVATE REPO CLONES ===" -ForegroundColor Cyan
$allDirs = Get-ChildItem 'C:\Users\sunilkr' -Directory -ErrorAction SilentlyContinue
foreach ($d in $allDirs) {
    $gitDir = Join-Path $d.FullName '.git'
    if (Test-Path $gitDir -ErrorAction SilentlyContinue) {
        Write-Host "  Git repo found: $($d.FullName)"
    }
}
