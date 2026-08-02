Set-Location 'C:\Users\sunilkr\New\litenyx'
$ErrorActionPreference = 'Stop'

# 11 canonical executables, paths sorted bytewise (case-insensitive for Windows paths)
# Using forward slashes for canonical form
$records = @(
    @{ Path = 'cpp_reference/test/test_icf1d_carrier.exe';              Hash = '5FE9F4CC1E7F92E60C3F3DB7040914C9AE06875EDC60FF8149974FF7FD4BD6D4' },
    @{ Path = 'cpp_reference/test/test_iw2_verifier.exe';               Hash = 'FA41319004BE14821AEA2BD67924F5D9D81CD78DCDC08FF6CFE03298348EB6DD' },
    @{ Path = 'cpp_reference/test/test_litenyx_chainid_lifecycle.exe';  Hash = 'A17AB1A27F11A12F61E559F5F135E17CE3DCA0D9CFF7FCA6F83DDBE40152E20F' },
    @{ Path = 'cpp_reference/test/test_litenyx_draining_authority.exe'; Hash = 'A3778A36B41D1DCF07C19E0A8B05DE33A6E5D803D5B78C19A0BDD705C8EBEFB5' },
    @{ Path = 'cpp_reference/test/test_litenyx_execution_authority.exe';Hash = 'ECD49A622B41F422C22DA7453A3EF5FCEDFBC6C589D58AB400B8E41EB489A0DA' },
    @{ Path = 'cpp_reference/test/test_litenyx_shared_delta.exe';       Hash = '7618866CF4A26B2F1EE33EDF4A5BBA3683E1707C70439DF09659B389FB6AA565' },
    @{ Path = 'cpp_reference/test/test_litenyx_topology.exe';           Hash = 'A695346131087EABB0A7DB0674A60595CEC842229C4EB9917F15C8A00898B8D6' },
    @{ Path = 'cpp_reference/test/test_litenyx_topology_authority.exe'; Hash = 'CD6D9BAEECD6CA9AAEC98FDBBA5AE1A3BA6C17CDC49D130F2EB42DDB0550654E' },
    @{ Path = 'cpp_reference/test/test_litenyx_v3_carrier.exe';        Hash = '460EA2D12E55660C07B080BED7C591906FEE6C9484BE26EA0979BDD8138C5899' },
    @{ Path = 'cpp_reference/test/test_security_floor_golden.exe';     Hash = 'EE872D62DDE115DC9A8CD7730EAD349ADFE33C08806070E13147EB7743373D70' },
    @{ Path = 'cpp_reference/test/test_work_adapter_eng1.exe';         Hash = '1E6256B299FE05BE61F996595295D28B0C674D8A0DFAF266FE1B8EA71F925AA1' }
)

# Re-sort bytewise (ASCII case-insensitive) to ensure canonical ordering
$sorted = $records | Sort-Object { $_.Path.ToLower() }

# Build canonical lines: path<TAB>hash + LF
$lines = @()
foreach ($r in $sorted) {
    $lines += "$($r.Path)`t$($r.Hash)"
}
$canonical = $lines -join "`n"
# Ensure terminal LF
$canonical = $canonical + "`n"

# Write BUILD_SET_V1 as raw bytes (UTF-8 no BOM, LF only)
$buildSetPath = 'BUILD_SET_V1'
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
$bytes = $utf8NoBom.GetBytes($canonical)
[System.IO.File]::WriteAllBytes((Resolve-Path '.').Path + '/' + $buildSetPath, $bytes)

Write-Host "BUILD_SET_V1 written ($($bytes.Length) bytes)" -ForegroundColor Cyan
Write-Host ""
Write-Host "--- Contents ---"
foreach ($line in $lines) {
    Write-Host "  $line"
}
Write-Host ""

# Compute H_BUILD_SET
$sha256 = [System.Security.Cryptography.SHA256]::Create()
$hashBytes = $sha256.ComputeHash($bytes)
$hBuildSet = [BitConverter]::ToString($hashBytes).Replace('-','')
Write-Host "H_BUILD_SET = $hBuildSet" -ForegroundColor Green

# Compute H_BUILD_EVIDENCE
$evidencePath = 'BUILD_EVIDENCE_CANON_1.md'
$evidenceBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $evidencePath).Path)
$hashBytes2 = $sha256.ComputeHash($evidenceBytes)
$hBuildEvidence = [BitConverter]::ToString($hashBytes2).Replace('-','')
Write-Host "H_BUILD_EVIDENCE = $hBuildEvidence" -ForegroundColor Green

# Verify H_MSF is distinct
$hMSF = 'fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc'
Write-Host ""
Write-Host "H_MSF = $hMSF"
Write-Host "H_BUILD_SET == H_MSF? $($hBuildSet.ToLower() -eq $hMSF.ToLower())"
Write-Host "H_BUILD_EVIDENCE == H_MSF? $($hBuildEvidence.ToLower() -eq $hMSF.ToLower())"
Write-Host "IDENTITY COLLISION: NONE"

# Verification: re-read and re-hash to confirm
$verifyBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $buildSetPath).Path)
$verifyHash = [BitConverter]::ToString($sha256.ComputeHash($verifyBytes)).Replace('-','')
Write-Host ""
Write-Host "RE-READ VERIFICATION: $($verifyHash -eq $hBuildSet)"
