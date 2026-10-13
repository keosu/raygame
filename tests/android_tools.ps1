$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../scripts/android-tools.ps1')
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../build'))
$fixture = Join-Path $workspace ("android-tools-tests-" + [guid]::NewGuid().ToString('N'))
$savedNdkHome = $env:ANDROID_NDK_HOME
$savedNdkRoot = $env:ANDROID_NDK_ROOT
$passed = 0

function Assert-ToolCheck([bool]$Condition, [string]$Description) {
    if (-not $Condition) { throw "FAIL: $Description" }
    $script:passed++
    Write-Output "PASS: $Description"
}

function Assert-ToolError([scriptblock]$Action, [string]$Message, [string]$Description) {
    try { & $Action | Out-Null }
    catch { Assert-ToolCheck ($_.Exception.Message.Contains($Message)) $Description; return }
    throw "FAIL: $Description (expected an error)"
}

function New-TestPackage([string]$Directory, [string[]]$Files, [string]$Revision) {
    foreach ($file in $Files) {
        $path = Join-Path $Directory $file
        New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
        [IO.File]::WriteAllText($path, '')
    }
    if ($Revision) { [IO.File]::WriteAllText((Join-Path $Directory 'source.properties'), "Pkg.Revision = $Revision`n") }
    return $Directory
}

try {
    $env:ANDROID_NDK_HOME = $null
    $env:ANDROID_NDK_ROOT = $null
    $sdk = New-TestPackage (Join-Path $fixture 'SDK with spaces') @('platforms/android-35/android.jar') ''
    $java = New-TestPackage (Join-Path $fixture 'JDK with spaces') @('bin/java.exe', 'bin/javac.exe', 'bin/keytool.exe') ''
    $ndkFiles = @('toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe',
                  'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe',
                  'sources/android/native_app_glue/android_native_app_glue.h')
    $buildFiles = @('aapt.exe', 'apksigner.bat', 'zipalign.exe', 'lib/d8.jar', 'lib/apksigner.jar')
    $ndk27 = New-TestPackage (Join-Path $sdk 'ndk/27.2.12479018') $ndkFiles '27.2.12479018'
    $build36 = New-TestPackage (Join-Path $sdk 'build-tools/36.0.0') $buildFiles '36.0.0'
    $config = Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java
    Assert-ToolCheck ($config.NdkRoot -eq $ndk27) 'an SDK containing only r27 works without r28c'
    Assert-ToolCheck ($config.BuildToolsRoot -eq $build36 -and $config.BuildToolsVersion -eq '36.0.0') 'Build Tools 36 works without 35.0.0'
    Assert-ToolCheck ($config.SdkRoot -eq $sdk -and $config.JavaHome -eq $java) 'paths containing spaces are preserved'
    $ndk29 = New-TestPackage (Join-Path $sdk 'ndk/29.0.14033849') $ndkFiles '29.0.14033849'
    New-TestPackage (Join-Path $sdk 'ndk/9.0.0') $ndkFiles '9.0.0' | Out-Null
    New-TestPackage (Join-Path $sdk 'ndk/30.0.0-beta1') $ndkFiles '30.0.0-beta1' | Out-Null
    New-TestPackage (Join-Path $sdk 'ndk/31.0.0') @($ndkFiles[0]) '31.0.0' | Out-Null
    New-TestPackage (Join-Path $sdk 'ndk/unfinished') $ndkFiles 'not-a-version' | Out-Null
    $build35 = New-TestPackage (Join-Path $sdk 'build-tools/35.0.0') $buildFiles '35.0.0'
    New-TestPackage (Join-Path $sdk 'build-tools/37.0.0') @('aapt.exe') '37.0.0' | Out-Null
    New-TestPackage (Join-Path $sdk 'build-tools/38.0.0-rc1') $buildFiles '38.0.0 rc1' | Out-Null
    $config = Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java
    Assert-ToolCheck ($config.NdkRoot -eq $ndk29) 'newest complete stable NDK is selected numerically; previews and incomplete installs are skipped'
    Assert-ToolCheck ($config.BuildToolsRoot -eq $build36) 'newest complete stable Build Tools is selected'
    $env:ANDROID_NDK_HOME = $ndk27
    $config = Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java
    Assert-ToolCheck ($config.NdkRoot -eq $ndk27) 'ANDROID_NDK_HOME overrides SDK discovery'
    $config = Resolve-AndroidTools -SdkRoot $sdk -NdkRoot $ndk29 -JavaHome $java -BuildToolsVersion '35.0.0'
    Assert-ToolCheck ($config.NdkRoot -eq $ndk29 -and $config.BuildToolsRoot -eq $build35) 'explicit NDK and Build Tools override automatic choices'
    $env:ANDROID_NDK_HOME = Join-Path $fixture 'stale-env'
    $env:ANDROID_NDK_ROOT = $ndk27
    Assert-ToolCheck ((Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java).NdkRoot -eq $ndk27) 'ANDROID_NDK_ROOT works after a stale NDK_HOME'
    $env:ANDROID_NDK_ROOT = $null
    Assert-ToolCheck ((Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java).NdkRoot -eq $ndk29) 'a stale environment hint does not prevent installed NDK discovery'
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $sdk -NdkRoot (Join-Path $fixture 'missing') -JavaHome $java } 'Required:' 'invalid explicit NDK fails with required files instead of silently falling back'
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java -BuildToolsVersion '99.0.0' } 'Android Build Tools path' 'missing explicit Build Tools fails clearly'
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java -BuildToolsVersion '../35.0.0' } 'stable version' 'Build Tools override rejects paths'
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $sdk -JavaHome $java -BuildToolsVersion '34.0.0' } '16 KiB ZIP alignment' 'unsupported Build Tools reports the alignment requirement'
    Assert-ToolError { Resolve-AndroidTools -SdkRoot (Join-Path $fixture 'bad-sdk') -JavaHome $java } 'platforms/android-35/android.jar' 'missing SDK platform reports the concrete requirement'
    $other = New-TestPackage (Join-Path $fixture 'Other SDK') @('platforms/android-35/android.jar') ''
    New-TestPackage (Join-Path $other 'build-tools/36.0.0') $buildFiles '36.0.0' | Out-Null
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $other -JavaHome $java } 'No complete stable Windows NDK' 'an SDK without NDKs gives an installation hint'
    $bundle = New-TestPackage (Join-Path $other 'ndk-bundle') $ndkFiles '26.3.11579264'
    Assert-ToolCheck ((Resolve-AndroidTools -SdkRoot $other -JavaHome $java).NdkRoot -eq $bundle) 'legacy ndk-bundle layout is discovered'
    $external = New-TestPackage (Join-Path $fixture 'External NDK') $ndkFiles '27.2.12479018'
    Assert-ToolCheck ((Resolve-AndroidTools -SdkRoot $other -NdkRoot $external -JavaHome $java).NdkVersion -eq '27.2.12479018') 'external custom directory reads its version from source.properties'
    $oldSdk = New-TestPackage (Join-Path $fixture 'Old SDK') @('platforms/android-35/android.jar') ''
    New-TestPackage (Join-Path $oldSdk 'ndk/27.2.12479018') $ndkFiles '' | Out-Null
    New-TestPackage (Join-Path $oldSdk 'build-tools/34.0.0') $buildFiles '34.0.0' | Out-Null
    Assert-ToolError { Resolve-AndroidTools -SdkRoot $oldSdk -JavaHome $java } 'Build Tools >= 35.0.0' 'only old Build Tools reports the missing supported component'
    Write-Output "Android tool discovery: $passed passed, 0 failed"
} finally {
    $env:ANDROID_NDK_HOME = $savedNdkHome
    $env:ANDROID_NDK_ROOT = $savedNdkRoot
    $resolvedFixture = [IO.Path]::GetFullPath($fixture)
    if (-not $resolvedFixture.StartsWith($workspace + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Test cleanup target escapes build/: $resolvedFixture"
    }
    if (Test-Path -LiteralPath $resolvedFixture) { Remove-Item -LiteralPath $resolvedFixture -Recurse -Force }
}
