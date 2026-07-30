$ErrorActionPreference = 'Continue'
$H_MSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'

$repos = @(
    'C:\Users\sunilkr\New\litenyx-plan',
    'C:\Users\sunilkr\New\litenyx-spec',
    'C:\Users\sunilkr\New\litenyx-walkthrough'
)

foreach ($repo in $repos) {
    if (-not (Test-Path $repo)) {
        Write-Host "=== $repo : NOT FOUND ===" -ForegroundColor Red
        continue
    }
    
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "REPO: $repo" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    
    Push-Location $repo
    
    $head = (git rev-parse HEAD 2>&1).Trim()
    $branch = (git branch --show-current 2>&1).Trim()
    Write-Host "  HEAD: $head"
    Write-Host "  Branch: $branch"
    
    Write-Host ""
    Write-Host "  --- H_MSF textual search ---"
    $grepResult = git grep -n -i $H_MSF 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  FOUND IN TREE:" -ForegroundColor Green
        Write-Host "  $grepResult"
    } else {
        Write-Host "  NOT in current tree"
    }
    
    $logResult = git log --all -S $H_MSF --oneline 2>&1
    if ($logResult -match '\w{7,40}') {
        Write-Host "  FOUND IN HISTORY:" -ForegroundColor Green
        Write-Host "  $logResult"
    } else {
        Write-Host "  NOT in history"
    }
    
    Write-Host ""
    Write-Host "  --- MSF/ORCH/scientific search ---"
    foreach ($term in @('MSF','ORCH','scientific','contract.*freeze','frozen.*spec','canonical.*hash')) {
        $res = git log --all --oneline --grep=$term -i 2>&1
        if ($res -match '\w{7,40}') {
            Write-Host "    '$term': $res" -ForegroundColor Green
        }
    }
    
    Write-Host ""
    Write-Host "  --- content search for fa003d ---"
    $files = git ls-files 2>&1
    $found = $false
    foreach ($file in $files) {
        if ($file -and (Test-Path $file -ErrorAction SilentlyContinue)) {
            $content = Get-Content -LiteralPath $file -Raw -ErrorAction SilentlyContinue
            if ($content -and $content -match 'fa003d') {
                Write-Host "    FOUND IN: $file" -ForegroundColor Green
                $found = $true
            }
        }
    }
    if (-not $found) { Write-Host "    NOT FOUND in file content" }
    
    Write-Host ""
    Write-Host "  --- tracked files ---"
    git ls-files 2>&1 | ForEach-Object { Write-Host "    $_" }
    
    Write-Host ""
    Write-Host "  --- branches ---"
    git branch -a 2>&1 | ForEach-Object { Write-Host "    $_" }
    
    Write-Host ""
    Write-Host "  --- tags ---"
    git tag -l 2>&1 | ForEach-Object { Write-Host "    $_" }
    
    # Check unreachable objects
    Write-Host ""
    Write-Host "  --- unreachable objects ---"
    $unr = git fsck --unreachable --no-reflogs 2>&1 | Select-String 'unreachable commit'
    if ($unr) {
        foreach ($line in $unr) {
            if ($line -match 'unreachable commit (\w+)') {
                $hash = $Matches[1]
                $log = git log $hash --oneline -1 2>&1
                Write-Host "    $log"
                $body = git log $hash -1 --format='%B' 2>&1
                if ($body -match 'MSF|fa003d|contract|ORCH') {
                    Write-Host "      RELEVANT BODY: $($body.Substring(0,[Math]::Min(300,$body.Length)))" -ForegroundColor Green
                }
            }
        }
    } else {
        Write-Host "    None"
    }
    
    Pop-Location
    Write-Host ""
}
