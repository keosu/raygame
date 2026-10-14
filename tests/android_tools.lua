-- Run with: xmake lua tests/android_tools.lua. No downloads, compiler, or devices.
import("scripts.android.tools", {rootdir = os.projectdir()})

function main()
    local workspace = path.join(os.projectdir(), "build/android-tools-tests")
    local fixture = path.join(workspace, hash.uuid4())
    local count = 0
    local files = tools.required()
    local function package(directory, required, version)
        directory = path.join(fixture, directory)
        for _, name in ipairs(required) do
            local file = path.join(directory, name)
            os.mkdir(path.directory(file))
            io.writefile(file, "")
        end
        if version then io.writefile(path.join(directory, "source.properties"), "Pkg.Revision = " .. version .. "\n") end
        return directory
    end
    local function check(condition, description)
        assert(condition, "FAIL: %s", description)
        count = count + 1
        print("PASS: %s", description)
    end
    local function failure(action, message, description)
        local caught
        try {action, catch {function (errors) caught = tostring(errors) end}}
        check(caught and caught:find(message, 1, true), description .. (caught and "" or " (expected error)"))
    end
    try {
        function ()
            local sdk = package("SDK with spaces", files.sdk)
            local jdk = package("JDK with spaces", files.jdk)
            local env = {}
            local opt = {sdk = sdk, jdk = jdk, env = env}
            local ndk27 = package("SDK with spaces/ndk/27.2.12479018", files.ndk, "27.2.12479018")
            local build36 = package("SDK with spaces/build-tools/36.0.0", files.build, "36.0.0")
            local resolved = tools.resolve(opt)
            check(resolved.ndk == ndk27, "SDK containing only r27 works without r28c")
            check(resolved.build == build36, "Build Tools 36 works without 35.0.0")
            check(resolved.sdk == sdk and resolved.jdk == jdk, "paths with spaces are preserved")
            local ndk29 = package("SDK with spaces/ndk/29.0.14033849", files.ndk, "29.0.14033849")
            package("SDK with spaces/ndk/9.0.0", files.ndk, "9.0.0")
            package("SDK with spaces/ndk/30.0.0-beta1", files.ndk, "30.0.0-beta1")
            package("SDK with spaces/ndk/31.0.0", {files.ndk[1]}, "31.0.0")
            package("SDK with spaces/ndk/unfinished", files.ndk, "not-a-version")
            local build35 = package("SDK with spaces/build-tools/35.0.0", files.build, "35.0.0")
            package("SDK with spaces/build-tools/37.0.0", {files.build[1]}, "37.0.0")
            package("SDK with spaces/build-tools/38.0.0-rc1", files.build, "38.0.0 rc1")
            check(tools.resolve(opt).ndk == ndk29, "numeric NDK ordering skips previews and partial installs")
            check(tools.resolve(opt).build == build36, "newest complete stable Build Tools selected")
            env.ANDROID_NDK_HOME = ndk27
            check(tools.resolve(opt).ndk == ndk27, "ANDROID_NDK_HOME overrides discovery")
            opt.ndk, opt["build-tools"] = ndk29, "35.0.0"
            resolved = tools.resolve(opt)
            check(resolved.ndk == ndk29 and resolved.build == build35, "explicit NDK/Build Tools override discovery")
            opt.ndk, opt["build-tools"] = nil, nil
            env.ANDROID_NDK_HOME, env.ANDROID_NDK_ROOT = path.join(fixture, "missing"), ndk27
            check(tools.resolve(opt).ndk == ndk27, "NDK_ROOT works after a stale NDK_HOME")
            env.ANDROID_NDK_ROOT = nil
            check(tools.resolve(opt).ndk == ndk29, "stale environment falls back to SDK")
            opt.ndk = path.join(fixture, "missing")
            failure(function () tools.resolve(opt) end, "Required:", "invalid explicit NDK reports requirements")
            opt.ndk, opt["build-tools"] = nil, "99.0.0"
            failure(function () tools.resolve(opt) end, "Android Build Tools path", "missing explicit Build Tools fails")
            opt["build-tools"] = "../35.0.0"
            failure(function () tools.resolve(opt) end, "stable version", "version option rejects directory traversal")
            opt["build-tools"] = "34.0.0"
            failure(function () tools.resolve(opt) end, "16 KiB ZIP alignment", "old tools report the alignment requirement")
            opt["build-tools"], opt.sdk = nil, path.join(fixture, "bad-sdk")
            failure(function () tools.resolve(opt) end, "platforms/android-35/android.jar", "missing SDK reports required platform")
            local other = package("Other SDK", files.sdk)
            package("Other SDK/build-tools/36.0.0", files.build, "36.0.0")
            opt.sdk = other
            failure(function () tools.resolve(opt) end, "No complete stable NDK", "SDK without NDK reports installation hint")
            local bundle = package("Other SDK/ndk-bundle", files.ndk, "26.3.11579264")
            check(tools.resolve(opt).ndk == bundle, "legacy ndk-bundle is discovered")
            opt.ndk = package("External NDK", files.ndk, "27.2.12479018")
            check(tools.resolve(opt).ndk_version == "27.2.12479018", "external NDK reads source.properties")
            opt.sdk = package("Old SDK", files.sdk)
            package("Old SDK/build-tools/34.0.0", files.build, "34.0.0")
            failure(function () tools.resolve(opt) end, "Build Tools >= 35.0.0", "old Build Tools fail discovery")
            print("Android tool discovery: %d passed, 0 failed", count)
        end,
        finally {function ()
            assert(path.directory(path.absolute(fixture)) == path.absolute(workspace))
            if os.isdir(fixture) then os.rm(fixture) end
        end}
    }
end
