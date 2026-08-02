Set-Location 'C:\Users\sunilkr\New\litenyx'

$CXX = 'g++'
$STD = '-std=c++20'
$FLAGS = '-O0', '-DKERRNYX_STANDALONE_TEST', '-DBOOST_TEST_DYN_LINK', '-mconsole'
$INCLUDES = '-I.', '-I./cpp_reference', '-I./external/dogecoin/src', '-I./external/dogecoin/src/config'
$BOOST_MT = '-lboost_unit_test_framework-mt', '-lpthread'

$testDir = 'cpp_reference/test'
$results = [System.Collections.ArrayList]::new()

function Build-And-Run {
    param(
        [string]$Name,
        [string]$Source,
        [string[]]$ExtraObjects = @(),
        [string[]]$ExtraFlags = @(),
        [string[]]$ExtraLink = @()
    )

    $bin = "$testDir/$Name"
    $src = "$testDir/$Source"

    Write-Host "`n=== $Name ===" -ForegroundColor Cyan

    $linkArgs = $BOOST_MT + $ExtraLink
    $compileArgs = @($STD) + $FLAGS + $ExtraFlags + $INCLUDES + @($src) + $ExtraObjects + @('-o', "$bin.exe") + $linkArgs

    $proc = Start-Process -FilePath $CXX -ArgumentList $compileArgs -NoNewWindow -Wait -PassThru -RedirectStandardError "$testDir\$Name.compile.err" -RedirectStandardOutput "$testDir\$Name.compile.out"
    $compileExit = $proc.ExitCode
    $compileErr = Get-Content "$testDir\$Name.compile.err" -ErrorAction SilentlyContinue
    if ($compileErr) { $compileErr | Where-Object { $_ -notmatch 'redefined|note:.*previous definition' } | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkYellow } }

    if ($compileExit -ne 0) {
        Write-Host "  COMPILE FAIL (exit $compileExit)" -ForegroundColor Red
        [void]$results.Add([PSCustomObject]@{ Name = $Name; Compile = 'FAIL'; Run = '-'; Status = 'FAIL' })
        return
    }

    $runProc = Start-Process -FilePath ".\$bin.exe" -ArgumentList '--log_level=test_suite' -NoNewWindow -Wait -PassThru -RedirectStandardOutput "$testDir\$Name.run.out" -RedirectStandardError "$testDir\$Name.run.err"
    $runExit = $runProc.ExitCode
    $runOut = Get-Content "$testDir\$Name.run.out" -ErrorAction SilentlyContinue
    $runErr = Get-Content "$testDir\$Name.run.err" -ErrorAction SilentlyContinue
    if ($runOut) { $runOut | ForEach-Object { Write-Host "  $_" } }
    if ($runErr) { $runErr | Where-Object { $_ -notmatch 'No errors detected' } | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkYellow } }

    $status = if ($runExit -eq 0) { 'PASS' } else { 'FAIL' }
    $color = if ($runExit -eq 0) { 'Green' } else { 'Red' }
    Write-Host "  => $status (exit $runExit)" -ForegroundColor $color

    [void]$results.Add([PSCustomObject]@{
        Name    = $Name
        Compile = 'OK'
        Run     = "exit=$runExit"
        Status  = $status
    })

    Remove-Item "$testDir\$Name.compile.err" -ErrorAction SilentlyContinue
    Remove-Item "$testDir\$Name.compile.out" -ErrorAction SilentlyContinue
    Remove-Item "$testDir\$Name.run.out" -ErrorAction SilentlyContinue
    Remove-Item "$testDir\$Name.run.err" -ErrorAction SilentlyContinue
}

# Phase 1: Security Floor (linked mode)
Build-And-Run -Name 'test_security_floor_golden' -Source 'test_security_floor_golden.cpp'

# Phase 3: Topology (linked mode)
Build-And-Run -Name 'test_litenyx_topology' -Source 'test_litenyx_topology.cpp'

# Phase 4A: Topology Authority (linked mode)
Build-And-Run -Name 'test_litenyx_topology_authority' -Source 'test_litenyx_topology_authority.cpp'

# Phase 5: ChainId Lifecycle (linked mode)
Build-And-Run -Name 'test_litenyx_chainid_lifecycle' -Source 'test_litenyx_chainid_lifecycle.cpp'

# Phase 5: V3 Carrier (linked mode)
Build-And-Run -Name 'test_litenyx_v3_carrier' -Source 'test_litenyx_v3_carrier.cpp'

# Phase 6: Execution Authority (linked mode)
Build-And-Run -Name 'test_litenyx_execution_authority' -Source 'test_litenyx_execution_authority.cpp'

# Phase 7: Draining Authority (linked mode)
Build-And-Run -Name 'test_litenyx_draining_authority' -Source 'test_litenyx_draining_authority.cpp'

# Shared Delta (needs Dogecoin support objects)
$dogeSupport = @('external/dogecoin/src/uint256.cpp', 'external/dogecoin/src/utilstrencodings.cpp')
Build-And-Run -Name 'test_litenyx_shared_delta' -Source 'test_litenyx_shared_delta.cpp' -ExtraObjects $dogeSupport -ExtraFlags @('-DHAVE_CONFIG_H')

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "PRIMARY HARNESS RESULTS" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
$results | Format-Table -AutoSize

$pass = ($results | Where-Object { $_.Status -eq 'PASS' }).Count
$fail = ($results | Where-Object { $_.Status -ne 'PASS' }).Count
Write-Host "PASS: $pass  FAIL: $fail  TOTAL: $($results.Count)" -ForegroundColor $(if ($fail -eq 0) { 'Green' } else { 'Red' })
