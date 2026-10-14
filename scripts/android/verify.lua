import("core.base.json")
import("tools")

function _elf(data, name)
    local function uint(offset, count)
        assert(offset + count <= #data, "Truncated ELF: %s", name)
        local value = 0
        for i = count - 1, 0, -1 do value = value * 256 + data:byte(offset + i + 1) end
        return value
    end
    assert(data:sub(1, 4) == "\127ELF" and uint(4, 1) == 2 and uint(5, 1) == 1 and uint(18, 2) == 183,
           "Expected little-endian ARM64 ELF: %s", name)
    local phoff, size, count = uint(32, 8), uint(54, 2), uint(56, 2)
    assert(size >= 56 and count > 0, "Invalid ELF program headers: %s", name)
    local loads = 0
    for i = 0, count - 1 do
        local offset = phoff + i * size
        if uint(offset, 4) == 1 then
            loads = loads + 1
            assert(uint(offset + 48, 8) >= 16384, "ELF needs 16 KiB segment alignment: %s", name)
        end
    end
    assert(loads > 0, "ELF has no LOAD segments: %s", name)
end

function main(selected, mode, resolved)
    local root = os.projectdir()
    local results = {}
    local java = path.join(resolved.jdk, "bin", tools.exe("java"))
    local jar = path.join(resolved.jdk, "bin", tools.exe("jar"))
    local aapt = path.join(resolved.build, tools.exe("aapt"))
    local readelf = path.join(resolved.ndk, "toolchains/llvm/prebuilt", tools.host_tag(), "bin", tools.exe("llvm-readelf"))
    for _, name in ipairs(selected) do
        local apk = path.join(root, "bin/android/arm64-v8a", mode, name .. ".apk")
        assert(os.isfile(apk), "Missing APK: %s", apk)
        os.vrunv(java, {"-jar", path.join(resolved.build, "lib/apksigner.jar"), "verify", "--verbose", apk})
        os.vrunv(path.join(resolved.build, tools.exe("zipalign")), {"-c", "-P", "16", "4", apk})
        local manifest = os.iorunv(aapt, {"dump", "xmltree", apk, "AndroidManifest.xml"})
        assert(manifest:find("android.intent.action.TTS_SERVICE", 1, true) and
               manifest:match("android:hasCode[^\r\n]*0xffffffff"), "Missing TTS manifest declarations: %s", name)
        local badging = os.iorunv(aapt, {"dump", "badging", apk})
        assert(badging:find("sdkVersion:'26'", 1, true) and badging:find("targetSdkVersion:'35'", 1, true),
               "Unexpected SDK requirements: %s", name)
        local entries, paths = {}, os.iorunv(jar, {"tf", apk})
        for entry in paths:gmatch("[^\r\n]+") do
            assert(not entry:find("\\", 1, true) and not entry:find(":", 1, true) and entry:sub(1, 1) ~= "/" and
                   not ("/" .. entry):find("/../", 1, true), "Unsafe APK entry: %s", entry)
            assert(not entries[entry], "Duplicate APK entry: %s", entry)
            entries[entry] = true
        end
        local work = path.join(root, "build/android/verification", mode, name)
        assert(path.directory(path.absolute(work)) == path.join(root, "build/android/verification", mode))
        if os.isdir(work) then os.rm(work) end
        os.mkdir(work)
        os.vrunv(jar, {"xf", apk}, {curdir = work})
        local dex = path.join(work, "classes.dex")
        assert(os.isfile(dex) and io.readfile(dex, {encoding = "binary"}):find("Lorg/raygame/SpeechBridge;", 1, true),
               "Missing SpeechBridge dex class: %s", name)
        local expected = {}
        local roots = name == "forge_runner" and {path.join(root, "examples", name, "assets")} or
            {path.join(root, "examples/common/assets"), path.join(root, "examples", name, "assets")}
        local prefix = name == "forge_runner" and "assets/forge/" or "assets/"
        for _, directory in ipairs(roots) do
            for _, source in ipairs(os.files(path.join(directory, "**"))) do
                local asset = prefix .. path.relative(source, directory):gsub("\\", "/")
                assert(not expected[asset], "Conflicting APK asset: %s", asset)
                expected[asset] = source
            end
        end
        if name == "forge_runner" then expected["assets/THIRD_PARTY_NOTICES.md"] = path.join(root, "examples/forge_runner/THIRD_PARTY_NOTICES.md") end
        local assets = {}
        for asset, source in pairs(expected) do
            assert(entries[asset], "Missing APK asset: %s/%s", name, asset)
            assert(hash.sha256(path.join(work, asset)) == hash.sha256(source), "APK asset differs: %s/%s", name, asset)
            table.insert(assets, asset)
        end
        for entry in pairs(entries) do
            if entry:sub(1, 7) == "assets/" and entry:sub(-1) ~= "/" then
                assert(expected[entry], "Unexpected APK asset: %s/%s", name, entry)
            end
        end
        for _, resource in ipairs({"res/mipmap-mdpi-v4/ic_launcher.png", "res/mipmap-anydpi-v26/ic_launcher.xml",
            "res/mipmap-anydpi-v33/ic_launcher.xml", "res/mipmap-xxxhdpi-v4/ic_launcher_foreground.png",
            "res/mipmap-xxxhdpi-v4/ic_launcher_monochrome.png"}) do
            assert(entries[resource], "Missing Android launcher icon: %s/%s", name, resource)
        end
        local so = path.join(work, "lib/arm64-v8a/lib" .. name .. ".so")
        assert(os.isfile(so), "Missing NativeActivity library: %s", name)
        _elf(io.readfile(so, {encoding = "binary"}), name)
        local symbols = os.iorunv(readelf, {"--dyn-syms", so})
        assert(symbols:match("GLOBAL%s+DEFAULT%s+%d+%s+ANativeActivity_onCreate"), "Missing exported NativeActivity entry: %s", name)
        local dynamic = os.iorunv(readelf, {"--dynamic", so})
        assert(not dynamic:find("libc++_shared", 1, true), "Unexpected shared C++ runtime: %s", name)
        table.sort(assets)
        table.insert(results, {Game = name, Apk = apk, Bytes = os.filesize(apk), SHA256 = hash.sha256(apk),
            ABI = "arm64-v8a", MinSdk = 26, TargetSdk = 35, Signing = "debug", RuntimeTested = false,
            Assets = assets, AssetContentsVerified = true, SpeechBridgeVerified = true,
            NdkVersion = resolved.ndk_version, BuildToolsVersion = resolved.build_version})
        print("Verified %s (%d bytes)", name, os.filesize(apk))
    end
    local report = path.join(root, "build/android/verification-" .. mode .. ".json")
    json.savefile(report, results)
    print("Android verification passed: %d APK(s); %s", #results, report)
end
