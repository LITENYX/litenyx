Set-Location 'C:\Users\sunilkr\New\litenyx'

Write-Host '=== PRIMARY HARNESS TEST TU CONFIGURATION ===' -ForegroundColor Cyan

$testDir = 'cpp_reference\test'

$matrix = Get-ChildItem -LiteralPath $testDir -Filter '*.cpp' -File |
    Sort-Object Name |
    ForEach-Object {
        $file = $_
        $content = Get-Content -LiteralPath $file.FullName -Raw

        [PSCustomObject]@{
            File             = $file.Name
            DefinesModule    = ([regex]::Matches($content, 'BOOST_TEST_MODULE')).Count
            IncludedRunner   = ([regex]::Matches($content, 'boost/test/included/unit_test\.hpp')).Count
            UnitTestHeader   = ([regex]::Matches($content, 'boost/test/unit_test\.hpp')).Count
            AutoTestCases    = ([regex]::Matches($content, 'BOOST_AUTO_TEST_CASE\s*\(')).Count
            FixtureTestCases = ([regex]::Matches($content, 'BOOST_FIXTURE_TEST_CASE\s*\(')).Count
            DataTestCases    = ([regex]::Matches($content, 'BOOST_DATA_TEST_CASE\s*\(')).Count
        }
    }

$matrix | Format-Table -AutoSize

$total = (
    ($matrix.AutoTestCases    | Measure-Object -Sum).Sum +
    ($matrix.FixtureTestCases | Measure-Object -Sum).Sum +
    ($matrix.DataTestCases    | Measure-Object -Sum).Sum
)

Write-Host "`nTOTAL REGISTRATION MACROS (cpp_reference/test): $total" -ForegroundColor Green

Write-Host "`n=== S1 TEST TU CONFIGURATION ===" -ForegroundColor Cyan

$s1Dir = 'cpp_reference\s1'

$s1Matrix = Get-ChildItem -LiteralPath $s1Dir -Filter 'test_*.cpp' -File |
    Sort-Object Name |
    ForEach-Object {
        $file = $_
        $content = Get-Content -LiteralPath $file.FullName -Raw

        [PSCustomObject]@{
            File           = $file.Name
            DefinesModule  = ([regex]::Matches($content, 'BOOST_TEST_MODULE')).Count
            IncludedRunner = ([regex]::Matches($content, 'boost/test/included/unit_test\.hpp')).Count
            UnitTestHeader = ([regex]::Matches($content, 'boost/test/unit_test\.hpp')).Count
            AutoTestCases  = ([regex]::Matches($content, 'BOOST_AUTO_TEST_CASE\s*\(')).Count
        }
    }

$s1Matrix | Format-Table -AutoSize

$s1Total = ($s1Matrix.AutoTestCases | Measure-Object -Sum).Sum
Write-Host "`nTOTAL REGISTRATION MACROS (cpp_reference/s1): $s1Total" -ForegroundColor Green

Write-Host "`n=== PRESERVING MACHINE-READABLE EVIDENCE ===" -ForegroundColor Cyan

$matrix   | ConvertTo-Json -Depth 3 | Set-Content '.\test_tu_matrix.json' -Encoding UTF8
$s1Matrix | ConvertTo-Json -Depth 3 | Set-Content '.\s1_test_tu_matrix.json' -Encoding UTF8

Write-Host "`n=== FILE HASHES ===" -ForegroundColor Cyan
Get-FileHash '.\test_tu_matrix.json' -Algorithm SHA256
Get-FileHash '.\s1_test_tu_matrix.json' -Algorithm SHA256

Write-Host "`n=== GIT STATE ===" -ForegroundColor Cyan
git rev-parse HEAD
git branch --show-current
git status --short
