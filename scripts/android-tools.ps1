# Tool discovery is separate from building so other SDK layouts can be checked without
# invoking xmake, generating keys or changing the caller's environment.

function Test-AndroidToolRoot([string]$Root, [string[]]$RequiredFiles) {
    if (-not $Root) { return $false }
    foreach ($file in $RequiredFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $Root $file) -PathType Leaf)) { return $false }
    }
    return $true
}

function Find-AndroidToolRoot([string]$Explicit, [string[]]$Candidates, [string[]]$RequiredFiles, [string]$Description) {
    if ($Explicit) {
        if (-not (Test-AndroidToolRoot $Explicit $RequiredFiles)) {
            throw "$Description path is incomplete or invalid: $Explicit. Required: $($RequiredFiles -join ', ')"
        }
        return (Resolve-Path -LiteralPath $Explicit).Path
    }
    foreach ($candidate in $Candidates) {
        if (Test-AndroidToolRoot $candidate $RequiredFiles) { return (Resolve-Path -LiteralPath $candidate).Path }
    }
    throw "$Description not found. See docs/ANDROID.md or provide its path explicitly. Searched: $($Candidates -join ', ')"
}

function Get-AndroidPackageRevision([string]$Root) {
    $properties = Join-Path $Root 'source.properties'
    $revision = Split-Path -Leaf $Root
    if (Test-Path -LiteralPath $properties -PathType Leaf) {
        foreach ($line in Get-Content -LiteralPath $properties) {
            if ($line -match '^\s*Pkg\.Revision\s*=\s*(.+?)\s*$') { $revision = $Matches[1]; break }
        }
    }
    if ($revision -notmatch '^(\d+\.\d+\.\d+(?:\.\d+)?)(.*)$') { return $null }
    [PSCustomObject]@{
        Version = [version]$Matches[1]
        Stable = $Matches[2].Trim().Length -eq 0
        Revision = $revision
        Root = (Resolve-Path -LiteralPath $Root).Path
    }
}

function Get-InstalledAndroidPackages([string]$Directory, [string[]]$RequiredFiles, [version]$Minimum = [version]'0.0.0') {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return }
    foreach ($entry in Get-ChildItem -LiteralPath $Directory -Directory) {
        if (-not (Test-AndroidToolRoot $entry.FullName $RequiredFiles)) { continue }
        $package = Get-AndroidPackageRevision $entry.FullName
        if ($package -and $package.Stable -and $package.Version -ge $Minimum) { $package }
    }
}

function Resolve-AndroidTools([string]$SdkRoot, [string]$NdkRoot, [string]$JavaHome, [string]$BuildToolsVersion) {
    $SdkRoot = Find-AndroidToolRoot $SdkRoot @(
        $env:ANDROID_HOME, $env:ANDROID_SDK_ROOT,
        "$env:LOCALAPPDATA/Android/Sdk", "$env:LOCALAPPDATA/raygame/android-sdk",
        "$env:USERPROFILE/.cache/balanceball-tools/android-sdk"
    ) @('platforms/android-35/android.jar') 'Android SDK platform 35'

    $ndkFiles = @('toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe',
                  'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe',
                  'sources/android/native_app_glue/android_native_app_glue.h')
    if ($NdkRoot) {
        $NdkRoot = Find-AndroidToolRoot $NdkRoot @() $ndkFiles 'Android NDK (-NdkRoot)'
    } else {
        $hint = @($env:ANDROID_NDK_HOME, $env:ANDROID_NDK_ROOT) |
            Where-Object { Test-AndroidToolRoot $_ $ndkFiles } | Select-Object -First 1
        if ($hint) {
            $NdkRoot = (Resolve-Path -LiteralPath $hint).Path
        } else {
            $installed = @(Get-InstalledAndroidPackages (Join-Path $SdkRoot 'ndk') $ndkFiles) |
                Sort-Object Version -Descending
            if ($installed.Count -gt 0) {
                $NdkRoot = $installed[0].Root
            } elseif (Test-AndroidToolRoot (Join-Path $SdkRoot 'ndk-bundle') $ndkFiles) {
                $NdkRoot = (Resolve-Path -LiteralPath (Join-Path $SdkRoot 'ndk-bundle')).Path
            } else {
                throw "No complete stable Windows NDK found in $SdkRoot/ndk or $SdkRoot/ndk-bundle. Install an NDK with SDK Manager, or pass -NdkRoot / set ANDROID_NDK_HOME."
            }
        }
    }
    $ndkPackage = Get-AndroidPackageRevision $NdkRoot
    if (-not $ndkPackage) { throw "Cannot read the NDK revision at $NdkRoot; check source.properties." }

    $buildFiles = @('aapt.exe', 'apksigner.bat', 'zipalign.exe', 'lib/d8.jar', 'lib/apksigner.jar')
    if ($BuildToolsVersion) {
        if ($BuildToolsVersion -notmatch '^\d+\.\d+\.\d+$' -or [version]$BuildToolsVersion -lt [version]'35.0.0') {
            throw '-BuildToolsVersion must be a stable version >= 35.0.0 (16 KiB ZIP alignment is required).'
        }
        $buildRoot = Find-AndroidToolRoot (Join-Path $SdkRoot "build-tools/$BuildToolsVersion") @() $buildFiles 'Android Build Tools'
    } else {
        $installed = @(Get-InstalledAndroidPackages (Join-Path $SdkRoot 'build-tools') $buildFiles ([version]'35.0.0')) |
            Sort-Object Version -Descending
        if ($installed.Count -eq 0) { throw "No complete stable Android Build Tools >= 35.0.0 found in $SdkRoot/build-tools. Install them with SDK Manager or pass -BuildToolsVersion." }
        $buildRoot = $installed[0].Root
        $BuildToolsVersion = Split-Path -Leaf $buildRoot
    }
    $javac = Get-Command javac.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    $pathJava = if ($javac) { Split-Path -Parent (Split-Path -Parent $javac.Source) }
    $JavaHome = Find-AndroidToolRoot $JavaHome @(
        $env:JAVA_HOME, $pathJava, "$env:USERPROFILE/.cache/balanceball-tools/jdk/jdk-17.0.18+8",
        "$env:LOCALAPPDATA/raygame/android-jdk17"
    ) @('bin/java.exe', 'bin/javac.exe', 'bin/keytool.exe') 'JDK 17+'
    [PSCustomObject]@{
        SdkRoot = $SdkRoot
        NdkRoot = $NdkRoot
        NdkVersion = $ndkPackage.Revision
        JavaHome = $JavaHome
        BuildToolsRoot = $buildRoot
        BuildToolsVersion = $BuildToolsVersion
    }
}
