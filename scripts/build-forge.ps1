param(
    [ValidateSet('debug','release')][string]$Mode='release',
    [switch]$Verify,
    [switch]$SkipGraphics
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$gameRoot=Join-Path $projectRoot 'examples/forge_runner'
$workRoot=Join-Path $projectRoot "build/forge/$Mode"
$sdkRoot=Join-Path $workRoot 'sdk'
$gameBuild=Join-Path $workRoot 'game-build'
$gameBin=Join-Path $gameBuild "windows/x64/$Mode"
$savedConfig=$env:XMAKE_CONFIGDIR
New-Item -ItemType Directory -Path $workRoot -Force | Out-Null
function Invoke-Xmake([string[]]$Arguments,[string]$Log) {
    & xmake @Arguments *> (Join-Path $workRoot $Log)
    if($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath (Join-Path $workRoot $Log) -Tail 35
        throw "xmake failed; see $workRoot/$Log"
    }
}
try {
    # Build/install the engine without altering the IDE's desktop configuration.
    $env:XMAKE_CONFIGDIR=Join-Path $workRoot 'engine-config'
    Push-Location $projectRoot
    try {
        Invoke-Xmake @('f','-P',$projectRoot,'-p','windows','-a','x64','--toolchain=msvc','-m',$Mode,
            '--build_examples=n','--build_tests=n','-o',(Join-Path $workRoot 'engine-build'),'-y') 'engine-config.log'
        Invoke-Xmake @('build','-P',$projectRoot,'-j','1','-y','raygame') 'engine-build.log'
        Invoke-Xmake @('install','-P',$projectRoot,'-o',$sdkRoot,'raygame') 'sdk-install.log'
    } finally { Pop-Location }
    $env:XMAKE_CONFIGDIR=Join-Path $workRoot 'game-config'
    Push-Location $gameRoot
    try {
        Invoke-Xmake @('f','-P',$gameRoot,'-p','windows','-a','x64','--toolchain=msvc','-m',$Mode,"--raygame_sdk=$sdkRoot",'-o',$gameBuild,'-y') 'game-config.log'
        Invoke-Xmake @('build','-P',$gameRoot,'-j','1','-y') 'game-build.log'
        if($Verify) {
            & (Join-Path $PSScriptRoot 'verify-windows-icons.ps1') -BinaryDirectory $gameBin `
                -Games @('forge_runner') -IconDirectory (Join-Path $gameRoot 'resources') -IconNames @('forge')
            Invoke-Xmake @('build','-P',$gameRoot,'-j','1','-y','forge_tests') 'tests-build.log'
            Invoke-Xmake @('run','-P',$gameRoot,'forge_tests',(Join-Path $gameRoot 'assets'),(Join-Path $workRoot 'test-data')) 'tests.log'
            Get-Content -LiteralPath (Join-Path $workRoot 'tests.log') -Tail 1
            if(-not $SkipGraphics) {
                $exe=Join-Path $gameBin 'forge_runner.exe'
                $data=Join-Path $workRoot 'smoke-data'
                $arguments=@('--smoke','--data',('"'+$data+'"'))
                $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
                if(-not $process.WaitForExit(60000)) {
                    $process.Kill(); $process.WaitForExit()
                    throw 'FORGE smoke exceeded 60 seconds; its test process was stopped'
                }
                if($process.ExitCode -ne 0){throw "FORGE smoke failed ($($process.ExitCode)); see $data/logs/game.log"}
                if((Get-Content -Raw -LiteralPath (Join-Path $data 'smoke-result.txt')).Trim() -ne '0'){throw 'FORGE reported smoke failures'}
                Write-Output "FORGE graphics smoke: 0 failures; screenshots: $data/screenshots"
            } else { Write-Output 'FORGE graphics smoke: not run (-SkipGraphics)' }
        }
    } finally { Pop-Location }
    Write-Output "FORGE executable: $gameBin/forge_runner.exe"
} finally {
    $env:XMAKE_CONFIGDIR=$savedConfig
}
