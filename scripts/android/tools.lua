-- Pure discovery: no downloads, key generation, configuration or environment edits.
import("lib.detect.find_tool")

function host_tag()
    return is_host("windows") and "windows-x86_64" or (is_host("macosx") and "darwin-x86_64" or "linux-x86_64")
end

function exe(name)
    return name .. (is_host("windows") and ".exe" or "")
end

function required()
    local native = "toolchains/llvm/prebuilt/" .. host_tag() .. "/bin/"
    return {
        sdk = {"platforms/android-35/android.jar"},
        ndk = {native .. exe("clang"), native .. exe("llvm-readelf"),
               "sources/android/native_app_glue/android_native_app_glue.h"},
        build = {exe("aapt"), exe("zipalign"), "apksigner" .. (is_host("windows") and ".bat" or ""),
                 "lib/d8.jar", "lib/apksigner.jar"},
        jdk = {"bin/" .. exe("java"), "bin/" .. exe("javac"), "bin/" .. exe("keytool"), "bin/" .. exe("jar")}
    }
end

function complete(root, files)
    if not root or root == "" then return false end
    for _, file in ipairs(files) do
        if not os.isfile(path.join(root, file)) then return false end
    end
    return true
end

function find_root(explicit, candidates, files, description)
    if explicit then
        assert(complete(explicit, files), "%s path is incomplete or invalid: %s. Required: %s",
               description, explicit, table.concat(files, ", "))
        return path.absolute(explicit)
    end
    for _, candidate in ipairs(candidates) do
        if complete(candidate, files) then return path.absolute(candidate) end
    end
    raise("%s not found. Run xmake android-setup or specify its path. Searched: %s", description, table.concat(candidates, ", "))
end

function revision(root)
    local properties = path.join(root, "source.properties")
    local value = os.isfile(properties) and (io.readfile(properties):match("Pkg%.Revision%s*=%s*([^\r\n]+)")) or path.filename(root)
    value = value and value:trim()
    if not value or not (value:match("^%d+%.%d+%.%d+$") or value:match("^%d+%.%d+%.%d+%.%d+$")) then return nil end
    local numbers = {}
    for number in value:gmatch("%d+") do table.insert(numbers, tonumber(number)) end
    return {root = path.absolute(root), version = value, numbers = numbers}
end

function newer(a, b)
    for i = 1, math.max(#a.numbers, #b.numbers) do
        local av, bv = a.numbers[i] or 0, b.numbers[i] or 0
        if av ~= bv then return av > bv end
    end
    return false
end

function installed(directory, files, minimum)
    local packages = {}
    for _, dir in ipairs(os.dirs(path.join(directory, "*"))) do
        local package = complete(dir, files) and revision(dir)
        if package and (not minimum or package.numbers[1] >= minimum) then table.insert(packages, package) end
    end
    table.sort(packages, newer)
    return packages
end

function java_candidates(opt)
    opt = opt or {}
    local env = opt.env or os.getenvs()
    local candidates = {env.JAVA_HOME or "", path.join(os.projectdir(), "build/android/tools/jdk")}
    if env.LOCALAPPDATA then table.insert(candidates, path.join(env.LOCALAPPDATA, "raygame/android-jdk17")) end
    if not opt.env then
        local javac = find_tool("javac")
        if javac then table.insert(candidates, path.directory(path.directory(javac.program))) end
    end
    return candidates
end

function resolve(opt)
    opt = opt or {}
    local env, files = opt.env or os.getenvs(), required()
    local sdk_candidates = {env.ANDROID_HOME or "", env.ANDROID_SDK_ROOT or "", env.ANDROID_SDK_HOME or "",
                            path.join(os.projectdir(), "build/android/tools/sdk")}
    if env.LOCALAPPDATA then
        table.insert(sdk_candidates, path.join(env.LOCALAPPDATA, "Android/Sdk"))
        table.insert(sdk_candidates, path.join(env.LOCALAPPDATA, "raygame/android-sdk"))
    end
    local userdir = env.HOME or env.USERPROFILE
    if userdir then
        table.insert(sdk_candidates, path.join(userdir, is_host("macosx") and "Library/Android/sdk" or "Android/Sdk"))
    end
    local saved = path.join(os.projectdir(), "build/android/setup.lua")
    local setup = os.isfile(saved) and io.load(saved) or {}
    if not opt.env and setup.sdk then table.insert(sdk_candidates, setup.sdk) end
    local sdk = find_root(opt.sdk, sdk_candidates, files.sdk, "Android SDK platform 35")
    local ndk = opt.ndk
    if not ndk then
        for _, hint in ipairs({env.ANDROID_NDK_HOME or "", env.ANDROID_NDK_ROOT or ""}) do
            if complete(hint, files.ndk) then ndk = hint; break end
        end
        if not ndk then
            local packages = installed(path.join(sdk, "ndk"), files.ndk)
            if #packages > 0 then ndk = packages[1].root
            elseif complete(path.join(sdk, "ndk-bundle"), files.ndk) then ndk = path.join(sdk, "ndk-bundle") end
        end
        assert(ndk, "No complete stable NDK found in %s/ndk or ndk-bundle. Run xmake android-setup or pass --ndk.", sdk)
    end
    ndk = find_root(ndk, {}, files.ndk, "Android NDK")
    -- Explicit versions may include a preview; automatic selection excludes them.
    local properties = path.join(ndk, "source.properties")
    local ndk_version = os.isfile(properties) and io.readfile(properties):match("Pkg%.Revision%s*=%s*([^\r\n]+)") or
                        (revision(ndk) and revision(ndk).version)
    assert(ndk_version, "Cannot read NDK revision from %s/source.properties", ndk)
    local build_version, build = opt["build-tools"]
    if build_version then
        assert(build_version:match("^%d+%.%d+%.%d+$") and tonumber(build_version:match("^%d+")) >= 35,
               "Build Tools must be a stable version >= 35.0.0 (16 KiB ZIP alignment is required).")
        build = find_root(path.join(sdk, "build-tools", build_version), {}, files.build, "Android Build Tools")
    else
        local packages = installed(path.join(sdk, "build-tools"), files.build, 35)
        assert(#packages > 0, "No complete stable Android Build Tools >= 35.0.0 found in %s/build-tools", sdk)
        build = packages[1].root
        build_version = path.filename(build)
    end
    local candidates = java_candidates(opt)
    if not opt.env and setup.jdk then table.insert(candidates, setup.jdk) end
    local jdk = find_root(opt.jdk, candidates, files.jdk, "JDK 17+")
    return {sdk = sdk, ndk = ndk, ndk_version = ndk_version:trim(), jdk = jdk, build = build, build_version = build_version}
end

function environment(tools)
    return {JAVA_HOME = tools.jdk, PATH = path.join(tools.jdk, "bin") .. path.envsep() .. (os.getenv("PATH") or "")}
end

function check_java(tools)
    local output = os.iorunv(path.join(tools.jdk, "bin", exe("javac")), {"-version"})
    local major = tonumber(output:match("javac%s+(%d+)"))
    assert(major and major >= 17, "JDK 17+ required; got %s", output:trim())
end
