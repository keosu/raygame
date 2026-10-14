import("net.http.download")
import("utils.archive.extract")
import("tools")

-- Pinned Microsoft OpenJDK archives (same hashes as xmake-repo openjdk 21.0.10).
local jdks = {
    ["windows-x64"] = {"windows-x64.zip", "45a44af1f832e720ea6ad90dd7b2c94a48b2e5bf2fab92b2403e975f78d7d5e1"},
    ["linux-x86_64"] = {"linux-x64.tar.gz", "18ca81d9cbca9b34eb3976c515310dc025efeb23c90616e3faa61c275acf60fd"},
    ["macosx-x86_64"] = {"macos-x64.tar.gz", "b4c94a74d06e1046480ebe35e01e038b532e966b10ff797dc8d530ee75fa560e"},
    ["macosx-arm64"] = {"macos-aarch64.tar.gz", "f480c5533d37564640fb831c6c1f3c674889bc5ffd363bb7019c69ea9335cf23"}
}
-- Google's repository2-1.xml checksums for cmdline-tools;19.0 (13114758).
local commands = {
    windows = {"win", "54a582f3bf73e04253602f2d1c80bd5868aac115"},
    linux = {"linux", "5fdcc763663eefb86a5b8879697aa6088b041e70"},
    macosx = {"mac", "c3e06a1959762e89167d1cbaa988605f6f7c1d24"}
}

function _unpack(url, checksum, staging, name, sha1)
    local archive = path.join(staging, name)
    local digest = sha1 and hash.sha1 or hash.sha256
    os.mkdir(staging)
    if not os.isfile(archive) or digest(archive):lower() ~= checksum then
        print("Downloading %s", url)
        download(url, archive)
    end
    assert(digest(archive):lower() == checksum, "Downloaded archive checksum mismatch: %s", archive)
    local unpacked = path.join(staging, "unpacked")
    assert(path.directory(path.absolute(unpacked)) == path.absolute(staging))
    if os.isdir(unpacked) then os.rm(unpacked) end
    extract(archive, unpacked)
    return unpacked, archive
end

function main(opt)
    local root = os.projectdir()
    local managed = path.join(root, "build/android/tools")
    local staging = path.join(root, "build/android/setup-downloads")
    local sdk = path.absolute(opt.sdk or path.join(managed, "sdk"))
    local files = tools.required()
    local jdk = opt.jdk
    if jdk then jdk = tools.find_root(jdk, {}, files.jdk, "JDK 17+") end
    if not jdk then
        for _, candidate in ipairs(tools.java_candidates()) do
            if tools.complete(candidate, files.jdk) then jdk = path.absolute(candidate); break end
        end
    end
    if not jdk then
        local spec = assert(jdks[os.host() .. "-" .. os.arch()], "Automatic JDK download is not supported on this host; pass --jdk")
        jdk = path.join(managed, "jdk")
        assert(not os.isdir(jdk), "Incomplete managed JDK at %s; move it aside before retrying", jdk)
        local unpacked, archive = _unpack("https://aka.ms/download-jdk/microsoft-jdk-21.0.10-" .. spec[1],
                                         spec[2], staging, "jdk-" .. spec[1])
        local homes = os.files(path.join(unpacked, "**/bin", tools.exe("javac")))
        assert(#homes == 1, "Expected one JDK in %s", unpacked)
        os.cp(path.directory(path.directory(homes[1])), jdk)
        assert(tools.complete(jdk, files.jdk), "Downloaded JDK is incomplete")
        os.rm(archive)
        os.rm(unpacked)
    end
    tools.check_java({jdk = jdk})
    local envs = tools.environment({jdk = jdk})
    local native = tools.installed(path.join(sdk, "ndk"), files.ndk)
    local build = tools.installed(path.join(sdk, "build-tools"), files.build, 35)
    local missing = {}
    if not tools.complete(sdk, files.sdk) then table.insert(missing, "platforms;android-35") end
    if #native == 0 and not tools.complete(path.join(sdk, "ndk-bundle"), files.ndk) then
        table.insert(missing, "ndk;28.2.13676358")
    end
    if #build == 0 then table.insert(missing, "build-tools;35.0.0") end
    if #missing > 0 then
        local manager_name = "sdkmanager" .. (is_host("windows") and ".bat" or "")
        local managers = os.files(path.join(sdk, "cmdline-tools/*/bin", manager_name))
        if #managers == 0 then
            local spec = assert(commands[os.host()], "Unsupported SDK manager host")
            local filename = "commandlinetools-" .. spec[1] .. "-13114758_latest.zip"
            local unpacked, archive = _unpack("https://dl.google.com/android/repository/" .. filename,
                                             spec[2], staging, filename, true)
            local destination = path.join(sdk, "cmdline-tools/19.0")
            assert(not os.isdir(destination), "Incomplete command-line tools at %s; move them aside before retrying", destination)
            os.mkdir(path.directory(destination))
            os.cp(path.join(unpacked, "cmdline-tools"), destination)
            managers = {path.join(destination, "bin", manager_name)}
            os.rm(archive)
            os.rm(unpacked)
        end
        local runopt = {envs = envs}
        if opt["accept-licenses"] then
            local answers = path.join(staging, "licenses.txt")
            os.mkdir(staging)
            io.writefile(answers, string.rep("y\n", 100))
            runopt.stdin = answers
        end
        -- Stream prompts/progress; runv would hide interactive license questions.
        os.execv(managers[1], {"--sdk_root=" .. sdk, "--licenses"}, runopt)
        os.execv(managers[1], table.join({"--sdk_root=" .. sdk}, missing), runopt)
    end
    local resolved = tools.resolve({sdk = sdk, jdk = jdk})
    os.mkdir(path.join(root, "build/android"))
    io.save(path.join(root, "build/android/setup.lua"), {sdk = sdk, jdk = jdk})
    print("Android tools ready. SDK: %s; NDK: %s; Build Tools: %s; JDK: %s",
          sdk, resolved.ndk_version, resolved.build_version, jdk)
    print("Run xmake android to build and verify APKs.")
end
