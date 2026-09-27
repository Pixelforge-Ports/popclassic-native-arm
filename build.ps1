param([switch]$NoCache)
$ErrorActionPreference = 'Stop'
$docker = Get-Command docker -ErrorAction SilentlyContinue
if (-not $docker) { throw 'Open Docker Desktop, then open a new PowerShell window and retry.' }
$dockerType = & $docker.Source info --format '{{.OSType}}'
if ($LASTEXITCODE -ne 0 -or $dockerType.Trim() -ne 'linux') {
    throw 'Start Docker Desktop with Linux containers enabled.'
}

Push-Location $PSScriptRoot
try {
    $dockerArgs = @('build', '--progress=plain', '-t', 'popclassic-build', '-f', 'portbase/Dockerfile.build')
    if ($NoCache) { $dockerArgs += '--no-cache' }
    $dockerArgs += 'portbase'
    & $docker.Source @dockerArgs
    if ($LASTEXITCODE -ne 0) { throw 'Docker build environment failed.' }

    $buildCommand = 'make -j2 && make libs && python3 tools/build_package.py'
    & $docker.Source run --rm --mount "type=bind,source=$PSScriptRoot,target=/src" -w /src popclassic-build bash -lc $buildCommand
    if ($LASTEXITCODE -ne 0) { throw 'Compilation or PortMaster packaging failed.' }
    Write-Host "Built: $PSScriptRoot/dist/popclassic.zip"
} finally {
    Pop-Location
}
