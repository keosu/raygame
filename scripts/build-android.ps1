param(
    [ValidateSet('all', 'space_shooter', 'bouncing_balls', 'neon_breakout', 'bullet_hell', 'forge_runner')]
    [string]$Game = 'all',
    [ValidateSet('debug', 'release')][string]$Mode = 'release',
    [string]$SdkRoot,
    [string]$NdkRoot,
    [string]$JavaHome,
    [ValidateRange(1, 32)][int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

function Find-ToolRoot($Explicit, $Candidates, $RequiredFile, $Description) {
    if ($Explicit) { $Candidates = @($Explicit) }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate $RequiredFile))) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "$Description not found. See docs/ANDROID.md or provide its path explicitly."
}

$SdkRoot = Find-ToolRoot $SdkRoot @(
    $env:ANDROID_HOME, $env:ANDROID_SDK_ROOT,
    "$env:LOCALAPPDATA/Android/Sdk", "$env:LOCALAPPDATA/raygame/android-sdk",
    "$env:USERPROFILE/.cache/balanceball-tools/android-sdk"
) 'platforms/android-35/android.jar' 'Android SDK 35'
$NdkRoot = Find-ToolRoot $NdkRoot @(
    $env:ANDROID_NDK_HOME, $env:ANDROID_NDK_ROOT,
    "$SdkRoot/ndk/28.2.13676358", "$env:LOCALAPPDATA/raygame/android-sdk/ndk/28.2.13676358"
) 'toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe' 'Android NDK (r28c recommended)'
$JavaHome = Find-ToolRoot $JavaHome @(
    $env:JAVA_HOME, "$env:USERPROFILE/.cache/balanceball-tools/jdk/jdk-17.0.18+8",
    "$env:LOCALAPPDATA/raygame/android-jdk17"
) 'bin/keytool.exe' 'JDK 17+'
$buildTools = Join-Path $SdkRoot 'build-tools/35.0.0'
foreach ($tool in @('aapt.exe', 'apksigner.bat', 'zipalign.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $buildTools $tool))) { throw "Missing Android build tool: $tool" }
}

$savedJava = $env:JAVA_HOME
$savedPath = $env:PATH
$savedConfig = $env:XMAKE_CONFIGDIR
Push-Location $projectRoot
try {
    $env:JAVA_HOME = $JavaHome
    $env:PATH = "$JavaHome/bin;$env:PATH"
    # Keep the IDE's desktop xmake configuration intact.
    $env:XMAKE_CONFIGDIR = Join-Path $projectRoot 'build/android/config'
    New-Item -ItemType Directory -Path build/android -Force | Out-Null
    $keystore = Join-Path $projectRoot 'build/android/debug.keystore'
    if (-not (Test-Path -LiteralPath $keystore)) {
        & "$JavaHome/bin/keytool.exe" -genkeypair -keystore $keystore -storepass android -keypass android `
            -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=RayGame Debug,O=RayGame,C=US'
        if ($LASTEXITCODE -ne 0) { throw 'Cannot generate the local debug signing key' }
    }
    & xmake f -p android -a arm64-v8a -m $Mode --toolchain=ndk "--ndk=$NdkRoot" "--android_sdk=$SdkRoot" `
        --ndk_sdkver=26 --build_toolver=35.0.0 --runtimes=c++_static --build_examples=y --build_tests=n "--android_keystore=$keystore" -o build/android -y
    if ($LASTEXITCODE -ne 0) { throw 'Android configure failed' }
    $buildArgs = @('build', '-j', $Jobs, '-y')
    if ($Game -ne 'all') { $buildArgs += $Game }
    & xmake @buildArgs
    if ($LASTEXITCODE -ne 0) { throw 'Android build failed' }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $names = if ($Game -eq 'all') { @('space_shooter','bouncing_balls','neon_breakout','bullet_hell','forge_runner') } else { @($Game) }
    $readelf = Join-Path $NdkRoot 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe'
    $results = foreach ($name in $names) {
        $apk = Join-Path $projectRoot "bin/android/arm64-v8a/$Mode/$name.apk"
        & "$buildTools/apksigner.bat" verify --verbose $apk | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Invalid APK signature: $name" }
        & "$buildTools/zipalign.exe" -c -P 16 4 $apk | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Invalid APK alignment: $name" }
        $archive = [IO.Compression.ZipFile]::OpenRead($apk)
        try {
            # Small games share fonts; FORGE owns its complete resource set.
            $expectedAssets = @{}
            $assetRoots = if ($name -eq 'forge_runner') { @("$projectRoot/examples/$name/assets") }
                          else { @("$projectRoot/examples/common/assets", "$projectRoot/examples/$name/assets") }
            $assetPrefix = if ($name -eq 'forge_runner') { 'assets/forge/' } else { 'assets/' }
            foreach ($assetRoot in $assetRoots) {
                $assetRoot = (Resolve-Path -LiteralPath $assetRoot).Path
                foreach ($file in (Get-ChildItem -LiteralPath $assetRoot -Recurse -File)) {
                    $relative = [IO.Path]::GetRelativePath($assetRoot, $file.FullName).Replace('\', '/')
                    $asset = "$assetPrefix$relative"
                    if ($expectedAssets.ContainsKey($asset)) { throw "Conflicting APK asset: $asset" }
                    $expectedAssets[$asset] = $file.FullName
                }
            }
            if ($name -eq 'forge_runner') {
                $expectedAssets['assets/THIRD_PARTY_NOTICES.md'] = "$projectRoot/examples/$name/THIRD_PARTY_NOTICES.md"
            }
            foreach ($asset in $expectedAssets.Keys) {
                $entry = $archive.GetEntry($asset)
                if (-not $entry) { throw "Missing APK asset: $asset" }
                $stream = $entry.Open()
                $sha256 = [Security.Cryptography.SHA256]::Create()
                try { $actualHash = [Convert]::ToHexString($sha256.ComputeHash($stream)) }
                finally { $stream.Dispose(); $sha256.Dispose() }
                if ($actualHash -ne (Get-FileHash -LiteralPath $expectedAssets[$asset] -Algorithm SHA256).Hash) {
                    throw "APK asset differs from its source: $name/$asset"
                }
            }
            foreach ($entry in $archive.Entries) {
                if ($entry.FullName.StartsWith('assets/') -and -not $entry.FullName.EndsWith('/') -and
                    -not $expectedAssets.ContainsKey($entry.FullName)) {
                    throw "Unexpected APK asset: $name/$($entry.FullName)"
                }
            }
            foreach ($resource in @('res/mipmap-mdpi-v4/ic_launcher.png',
                                    'res/mipmap-anydpi-v26/ic_launcher.xml',
                                    'res/mipmap-anydpi-v33/ic_launcher.xml',
                                    'res/mipmap-xxxhdpi-v4/ic_launcher_foreground.png',
                                    'res/mipmap-xxxhdpi-v4/ic_launcher_monochrome.png')) {
                if (-not $archive.GetEntry($resource)) { throw "Missing Android launcher icon: $resource" }
            }
            $native = $archive.GetEntry("lib/arm64-v8a/lib$name.so")
            if (-not $native) { throw "Missing NativeActivity library: $name" }
            $stream = $native.Open()
            $memory = [IO.MemoryStream]::new()
            try { $stream.CopyTo($memory); $elf = $memory.ToArray() }
            finally { $stream.Dispose(); $memory.Dispose() }
            if ($elf[4] -ne 2 -or [BitConverter]::ToUInt16($elf, 18) -ne 183) { throw "Expected ARM64 ELF: $name" }
            $phoff = [BitConverter]::ToUInt64($elf, 32)
            $phsize = [BitConverter]::ToUInt16($elf, 54)
            $phcount = [BitConverter]::ToUInt16($elf, 56)
            for ($i = 0; $i -lt $phcount; $i++) {
                $offset = [int]($phoff + $i * $phsize)
                if ([BitConverter]::ToUInt32($elf, $offset) -eq 1 -and
                    [BitConverter]::ToUInt64($elf, $offset + 48) -lt 16384) { throw "ELF needs 16 KiB segment alignment: $name" }
            }
        } finally { $archive.Dispose() }
        $so = Join-Path $projectRoot "bin/android/arm64-v8a/$Mode/lib$name.so"
        $symbols = (& $readelf --dyn-syms $so) -join "`n"
        if ($LASTEXITCODE -ne 0 -or $symbols -notmatch 'GLOBAL\s+DEFAULT\s+\d+\s+ANativeActivity_onCreate') {
            throw "Missing exported NativeActivity entry: $name"
        }
        $dynamic = (& $readelf --dynamic $so) -join "`n"
        if ($LASTEXITCODE -ne 0 -or $dynamic -match 'libc\+\+_shared') { throw "Unexpected C++ shared runtime dependency: $name" }
        [PSCustomObject]@{
            Game = $name; Apk = $apk; Bytes = (Get-Item -LiteralPath $apk).Length
            SHA256 = (Get-FileHash -LiteralPath $apk -Algorithm SHA256).Hash
            ABI = 'arm64-v8a'; MinSdk = 26; TargetSdk = 35; Signing = 'debug'; RuntimeTested = $false
            Assets = @($expectedAssets.Keys | Sort-Object); AssetContentsVerified = $true
        }
    }
    $results | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath "build/android/verification-$Mode.json" -Encoding utf8
    $results | Format-Table Game,Bytes,Apk -AutoSize
} finally {
    $env:JAVA_HOME = $savedJava
    $env:PATH = $savedPath
    $env:XMAKE_CONFIGDIR = $savedConfig
    Pop-Location
}
