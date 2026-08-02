Set-Location 'C:\Users\sunilkr\New\litenyx'

Write-Host '=== WORK ADAPTER PROVENANCE ===' -ForegroundColor Cyan

git log --all --follow --oneline -- 'cpp_reference/test/test_work_adapter_eng1.cpp'

Write-Host "`n=== WORK ADAPTER FULL GIT HISTORY ===" -ForegroundColor Cyan

git log --all --follow --name-status -- 'cpp_reference/test/test_work_adapter_eng1.cpp'

Write-Host "`n=== BUILD REFERENCES (repo-wide grep) ===" -ForegroundColor Cyan

git grep -n -E 'test_work_adapter_eng1|work_adapter_eng1|test_litenyx\.cpp|harness_test'

Write-Host "`n=== RUNNER STRUCTURE ===" -ForegroundColor Cyan

Select-String `
    -LiteralPath 'cpp_reference/test/test_work_adapter_eng1.cpp' `
    -Pattern 'BOOST_TEST_MODULE','boost/test/included/unit_test.hpp','boost/test/unit_test.hpp','BOOST_AUTO_TEST_CASE','#include.*test_.*\.cpp','main\s*\('

Write-Host "`n=== FILE IDENTITY ===" -ForegroundColor Cyan

Get-Item 'cpp_reference/test/test_work_adapter_eng1.cpp' |
    Select-Object FullName,Length,LastWriteTimeUtc

Get-FileHash 'cpp_reference/test/test_work_adapter_eng1.cpp' -Algorithm SHA256

Write-Host "`n=== TEST_LITENYX FILE IDENTITY ===" -ForegroundColor Cyan

Get-Item 'cpp_reference/test/test_litenyx.cpp' |
    Select-Object FullName,Length,LastWriteTimeUtc

Get-FileHash 'cpp_reference/test/test_litenyx.cpp' -Algorithm SHA256

Write-Host "`n=== HARNESS INFRASTRUCTURE HEADERS ===" -ForegroundColor Cyan

Get-ChildItem 'harness\include' -Filter '*.h' -File |
    Sort-Object Name |
    ForEach-Object {
        [PSCustomObject]@{
            File   = $_.Name
            Length = $_.Length
            Hash   = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        }
    } | Format-Table -AutoSize

Write-Host "`n=== HARNESS INFRASTRUCTURE SOURCES ===" -ForegroundColor Cyan

Get-ChildItem 'harness\src' -Filter '*.cpp' -File |
    Sort-Object Name |
    ForEach-Object {
        [PSCustomObject]@{
            File   = $_.Name
            Length = $_.Length
            Hash   = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        }
    } | Format-Table -AutoSize

Write-Host "`n=== BUILD FILE SEARCH ===" -ForegroundColor Cyan

$buildFiles = Get-ChildItem . -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object {
        $_.Name -match '^(CMakeLists\.txt|Makefile.*)$' -or
        $_.Extension -in '.ps1','.sh','.bat','.cmd','.yml','.yaml','.vcxproj'
    }

if ($buildFiles) {
    $buildFiles |
        Select-String -Pattern 'test_work_adapter_eng1','test_litenyx','harness_test' -ErrorAction SilentlyContinue |
        Select-Object Path,LineNumber,Line
} else {
    Write-Host 'NO BUILD FILES FOUND IN REPOSITORY' -ForegroundColor Yellow
}

Write-Host "`n=== WORK ADAPTER INCLUDES (dependency scan) ===" -ForegroundColor Cyan

Select-String -LiteralPath 'cpp_reference/test/test_work_adapter_eng1.cpp' -Pattern '#include' |
    ForEach-Object { '{0}: {1}' -f $_.LineNumber, $_.Line.Trim() }

Write-Host "`n=== WORK ADAPTER HEADERS REFERENCED ===" -ForegroundColor Cyan

Select-String -LiteralPath 'cpp_reference/test/test_work_adapter_eng1.cpp' -Pattern 'harness/' |
    ForEach-Object { '{0}: {1}' -f $_.LineNumber, $_.Line.Trim() }
