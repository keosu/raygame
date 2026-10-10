param([string]$BinaryDirectory = 'bin')
$ErrorActionPreference = 'Stop'

# Read resources as data without executing game entry points or loading dependencies.
if (-not ('RayGameIconResources' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class RayGameIconResources {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr LoadLibraryExW(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr module);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindResourceW(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll")]
    public static extern uint SizeofResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern IntPtr LockResource(IntPtr resource);
}
'@
}

function Read-IconResource([IntPtr]$Module, [int]$Id, [int]$Type) {
    $resource = [RayGameIconResources]::FindResourceW($Module, [IntPtr]$Id, [IntPtr]$Type)
    if ($resource -eq [IntPtr]::Zero) { throw "Missing icon resource $Type/$Id" }
    $size = [RayGameIconResources]::SizeofResource($Module, $resource)
    $data = [RayGameIconResources]::LockResource([RayGameIconResources]::LoadResource($Module, $resource))
    if ($data -eq [IntPtr]::Zero -or $size -eq 0) { throw 'Cannot read icon resource' }
    $bytes = [byte[]]::new($size)
    [Runtime.InteropServices.Marshal]::Copy($data, $bytes, 0, $size)
    return ,$bytes
}

$projectRoot = Split-Path -Parent $PSScriptRoot
foreach ($game in @('space_shooter', 'bouncing_balls', 'neon_breakout', 'bullet_hell')) {
    $exe = (Resolve-Path -LiteralPath (Join-Path $BinaryDirectory "$game.exe")).Path
    $module = [RayGameIconResources]::LoadLibraryExW($exe, [IntPtr]::Zero, 0x22)
    if ($module -eq [IntPtr]::Zero) { throw "Cannot open EXE resources: $exe" }
    try {
        $group = Read-IconResource $module 1 14
        $ico = [IO.File]::ReadAllBytes((Join-Path $projectRoot "examples/resources/windows/$game.ico"))
        $count = [BitConverter]::ToUInt16($group, 4)
        if ($count -ne 8 -or $count -ne [BitConverter]::ToUInt16($ico, 4)) { throw "Expected eight icon sizes: $game" }
        for ($i = 0; $i -lt $count; $i++) {
            $entry = 6 + $i * 16
            $length = [BitConverter]::ToUInt32($ico, $entry + 8)
            $offset = [BitConverter]::ToUInt32($ico, $entry + 12)
            $id = [BitConverter]::ToUInt16($group, 6 + $i * 14 + 12)
            $actual = Read-IconResource $module $id 3
            if ([Convert]::ToBase64String($actual) -ne [Convert]::ToBase64String($ico, $offset, $length)) {
                throw "Embedded icon differs from the game's artwork: $game, size index $i"
            }
        }
        "$game.exe: eight embedded icon sizes match the original ICO"
    } finally { [void][RayGameIconResources]::FreeLibrary($module) }
}
