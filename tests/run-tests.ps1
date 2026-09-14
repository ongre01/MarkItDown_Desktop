[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [ValidateSet('unit', 'component', 'ui', 'all')]
    [string]$Category = 'all'
)

$ErrorActionPreference = 'Stop'

$resolvedBuildDirectory =
    (Resolve-Path -LiteralPath $BuildDirectory -ErrorAction Stop).Path

$testTargets = [ordered]@{
    unit = @(
        'tst_DocumentFileOperations',
        'tst_MarkdownRenderState',
        'tst_ConversionErrorPresentation',
        'tst_MarkdownDocumentRenderer'
    )
    component = @(
        'tst_MarkItDownManager',
        'tst_DocumentController'
    )
    ui = @(
        'tst_MainWindow'
    )
}

$categories =
    if ($Category -eq 'all') {
        @('unit', 'component', 'ui')
    }
    else {
        @($Category)
    }

$failedTargets = [System.Collections.Generic.List[string]]::new()

foreach ($testCategory in $categories) {
    $categoryBuildDirectory =
        Join-Path $resolvedBuildDirectory $testCategory
    $wrapperPath =
        Join-Path $categoryBuildDirectory 'target_wrapper.bat'

    if (-not (Test-Path -LiteralPath $wrapperPath -PathType Leaf)) {
        throw "Qt test wrapper not found. Build the tests first: $wrapperPath"
    }

    Write-Output "== $testCategory tests =="

    foreach ($target in $testTargets[$testCategory]) {
        $executablePath =
            Join-Path $categoryBuildDirectory "bin\$target.exe"

        if (-not (Test-Path -LiteralPath $executablePath -PathType Leaf)) {
            throw "Test executable not found. Build the tests first: $executablePath"
        }

        $logPath = Join-Path `
            ([System.IO.Path]::GetTempPath()) `
            ("MarkItDown_{0}_{1}.txt" -f $target, [guid]::NewGuid())

        Write-Output "-- $target"

        try {
            & $wrapperPath $executablePath '-o' "$logPath,txt"
            $testExitCode = $LASTEXITCODE

            if (Test-Path -LiteralPath $logPath -PathType Leaf) {
                Get-Content -LiteralPath $logPath -Encoding UTF8
            }
            else {
                Write-Warning "Qt Test did not create its text log: $target"
            }

            if ($testExitCode -ne 0) {
                $failedTargets.Add("$testCategory/$target ($testExitCode)")
            }
        }
        finally {
            if (Test-Path -LiteralPath $logPath -PathType Leaf) {
                Remove-Item -LiteralPath $logPath -Force
            }
        }
    }
}

if ($failedTargets.Count -ne 0) {
    throw (
        "Test run failed: {0}" -f ($failedTargets -join ', ')
    )
}

Write-Output "All selected tests passed: $Category"
