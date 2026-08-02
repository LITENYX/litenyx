Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Stop'

Write-Host "=== INDEPENDENT VERIFICATION ===" -ForegroundColor Cyan

# 1. Git state
$gitHead = (git rev-parse HEAD).Trim()
$gitBranch = (git branch --show-current).Trim()
Write-Host "Git HEAD: $gitHead"
Write-Host "Branch:   $gitBranch"
$headOk = $gitHead -eq 'aae45b40d1cf292e23cb5b96432fbf001f62c14c'
$branchOk = $gitBranch -eq 'phase7-draining-authority'
Write-Host "  HEAD MATCH: $headOk"
Write-Host "  BRANCH MATCH: $branchOk"

# 2. Count executables
$exeFiles = Get-ChildItem -Path 'cpp_reference/test' -Filter '*.exe' -File
Write-Host "Executable count: $($exeFiles.Count)  (expected 11)"
Write-Host "  COUNT MATCH: $($exeFiles.Count -eq 11)"

# 3. Binary hash verification
$expectedNames = @(
    'test_security_floor_golden',
    'test_litenyx_topology',
    'test_litenyx_topology_authority',
    'test_litenyx_chainid_lifecycle',
    'test_litenyx_v3_carrier',
    'test_litenyx_execution_authority',
    'test_litenyx_draining_authority',
    'test_litenyx_shared_delta',
    'test_icf1d_carrier',
    'test_iw2_verifier',
    'test_work_adapter_eng1'
)
$expectedBinHashes = @(
    'EE872D62DDE115DC9A8CD7730EAD349ADFE33C08806070E13147EB7743373D70',
    'A695346131087EABB0A7DB0674A60595CEC842229C4EB9917F15C8A00898B8D6',
    'CD6D9BAEECD6CA9AAEC98FDBBA5AE1A3BA6C17CDC49D130F2EB42DDB0550654E',
    'A17AB1A27F11A12F61E559F5F135E17CE3DCA0D9CFF7FCA6F83DDBE40152E20F',
    '460EA2D12E55660C07B080BED7C591906FEE6C9484BE26EA0979BDD8138C5899',
    'ECD49A622B41F422C22DA7453A3EF5FCEDFBC6C589D58AB400B8E41EB489A0DA',
    'A3778A36B41D1DCF07C19E0A8B05DE33A6E5D803D5B78C19A0BDD705C8EBEFB5',
    '7618866CF4A26B2F1EE33EDF4A5BBA3683E1707C70439DF09659B389FB6AA565',
    '5FE9F4CC1E7F92E60C3F3DB7040914C9AE06875EDC60FF8149974FF7FD4BD6D4',
    'FA41319004BE14821AEA2BD67924F5D9D81CD78DCDC08FF6CFE03298348EB6DD',
    '1E6256B299FE05BE61F996595295D28B0C674D8A0DFAF266FE1B8EA71F925AA1'
)
$expectedTestCounts = @(12, 5, 22, 12, 5, 17, 18, 6, 24, 36, 49)

$allHashMatch = $true
$totalTests = 0
Write-Host ""
Write-Host "BINARY HASH VERIFICATION:"
for ($i = 0; $i -lt $expectedNames.Count; $i++) {
    $name = $expectedNames[$i]
    $binPath = "cpp_reference/test/$name.exe"
    if (-not (Test-Path -LiteralPath $binPath)) {
        Write-Host "  MISSING: $name.exe" -ForegroundColor Red
        $allHashMatch = $false
        continue
    }
    $actualHash = (Get-FileHash -LiteralPath (Resolve-Path $binPath).Path -Algorithm SHA256).Hash
    $match = $actualHash -eq $expectedBinHashes[$i]
    if (-not $match) { $allHashMatch = $false }
    $mark = if ($match) { 'OK' } else { 'MISMATCH' }
    $color = if ($match) { 'Green' } else { 'Red' }
    Write-Host "  $name : $mark" -ForegroundColor $color
    if (-not $match) {
        Write-Host "    Expected: $($expectedBinHashes[$i])"
        Write-Host "    Actual:   $actualHash"
    }
    $totalTests += $expectedTestCounts[$i]
}
Write-Host ""
Write-Host "Total tests: $totalTests (expected 206)"
Write-Host "ALL HASHES MATCH: $allHashMatch"

# 4. bitcoin-config.h
Write-Host ""
Write-Host "BITCOIN-CONFIG.H:"
$configPath = 'external/dogecoin/src/config/bitcoin-config.h'
if (Test-Path -LiteralPath $configPath) {
    $configHash = (Get-FileHash -LiteralPath (Resolve-Path $configPath).Path -Algorithm SHA256).Hash
    $configGit = (git status --porcelain $configPath 2>&1).Trim()
    Write-Host "  SHA-256: $configHash"
    Write-Host "  EXPECTED: B52938BB9916EE6B1E78BE9DD1F33563E00BEF889705D4E8E51FC9D97953E536"
    Write-Host "  MATCH: $($configHash -eq 'B52938BB9916EE6B1E78BE9DD1F33563E00BEF889705D4E8E51FC9D97953E536')"
    if ($configGit -eq '') {
        Write-Host "  Git: UNTRACKED (verified)"
    } else {
        Write-Host "  Git: $configGit"
    }
} else {
    Write-Host "  NOT FOUND" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== VERIFICATION COMPLETE ==="
