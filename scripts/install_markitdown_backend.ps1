[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $Destination,

    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $RequirementsFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string] $FilePath,

        [Parameter(Mandatory = $true)]
        [string[]] $Arguments,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

function Find-PythonLauncher {
    $minimumVersionCheck =
        'import sys; raise SystemExit(0 if sys.version_info >= (3, 10) else 1)'

    $candidates = @(
        [pscustomobject]@{ Command = 'py.exe'; Prefix = @('-3.12') },
        [pscustomobject]@{ Command = 'py.exe'; Prefix = @('-3') },
        [pscustomobject]@{ Command = 'python.exe'; Prefix = @() },
        [pscustomobject]@{ Command = 'python3.exe'; Prefix = @() }
    )

    foreach ($candidate in $candidates) {
        $command = Get-Command $candidate.Command -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (-not $command) {
            continue
        }

        $arguments = @($candidate.Prefix) + @('-c', $minimumVersionCheck)
        & $command.Source @arguments *> $null
        if ($LASTEXITCODE -eq 0) {
            return [pscustomobject]@{
                FilePath = $command.Source
                Prefix = @($candidate.Prefix)
            }
        }
    }

    throw 'Python 3.10 or newer was not found. Install Python and rebuild the application.'
}

$resolvedRequirements = [System.IO.Path]::GetFullPath($RequirementsFile)
if (-not (Test-Path -LiteralPath $resolvedRequirements -PathType Leaf)) {
    throw "MarkItDown requirements file was not found: $resolvedRequirements"
}

$resolvedDestination = [System.IO.Path]::GetFullPath($Destination)
$venvPython = Join-Path $resolvedDestination 'Scripts\python.exe'
$markItDownExecutable = Join-Path $resolvedDestination 'Scripts\markitdown.exe'

if (-not (Test-Path -LiteralPath $venvPython -PathType Leaf)) {
    $launcher = Find-PythonLauncher
    $venvArguments = @($launcher.Prefix) + @('-m', 'venv', $resolvedDestination)
    Invoke-NativeCommand `
        -FilePath $launcher.FilePath `
        -Arguments $venvArguments `
        -Description 'Creating the application-local Python environment'
}

Invoke-NativeCommand `
    -FilePath $venvPython `
    -Arguments @(
        '-m', 'pip', 'install',
        '--disable-pip-version-check',
        '--quiet',
        '--requirement', $resolvedRequirements
    ) `
    -Description 'Installing the application-local MarkItDown backend'

if (-not (Test-Path -LiteralPath $markItDownExecutable -PathType Leaf)) {
    throw "MarkItDown CLI was not created: $markItDownExecutable"
}

& $markItDownExecutable --help *> $null
if ($LASTEXITCODE -ne 0) {
    throw "MarkItDown CLI verification failed with exit code $LASTEXITCODE."
}

Write-Host "MarkItDown backend is ready: $markItDownExecutable"
