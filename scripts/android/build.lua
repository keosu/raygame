import("tools")
import("verify")

function main(opt)
    local games = {"space_shooter", "bouncing_balls", "neon_breakout", "bullet_hell", "forge_runner",
                   "engine_lab", "marble_temple", "garden_guard"}
    local mode, game = opt.mode or "release", opt.game or "all"
    local jobs = tonumber(opt.jobs or "4")
    assert(mode == "debug" or mode == "release", "Mode must be debug or release")
    assert(game == "all" or table.contains(games, game), "Unknown Android game: %s", game)
    assert(jobs and jobs >= 1 and jobs <= 32 and jobs % 1 == 0, "Jobs must be an integer from 1 to 32")
    local root = os.projectdir()
    local report = path.join(root, "build/android/verification-" .. mode .. ".json")
    if os.isfile(report) then os.rm(report) end
    local resolved = tools.resolve(opt)
    tools.check_java(resolved)
    print("Android SDK: %s\nNDK: %s (%s)\nBuild Tools: %s\nJDK: %s",
          resolved.sdk, resolved.ndk_version, resolved.ndk, resolved.build_version, resolved.jdk)
    if not opt["verify-only"] then
        -- Per-child environment overrides: the caller and IDE configuration stay intact,
        -- including when a tool fails. All xmake invocations explicitly select this project.
        local envs = tools.environment(resolved)
        envs.XMAKE_CONFIGDIR = path.join(root, "build/android/config")
        local key = path.join(root, "build/android/debug.keystore")
        if not os.isfile(key) then
            os.mkdir(path.directory(key))
            os.vrunv(path.join(resolved.jdk, "bin", tools.exe("keytool")), {"-genkeypair", "-keystore", key,
                "-storepass", "android", "-keypass", "android", "-alias", "androiddebugkey", "-keyalg", "RSA",
                "-keysize", "2048", "-validity", "10000", "-dname", "CN=RayGame Debug,O=RayGame,C=US"}, {envs = envs})
        end
        os.execv(os.programfile(), {"f", "-P", root, "-p", "android", "-a", "arm64-v8a", "-m", mode,
            "--toolchain=ndk", "--ndk=" .. resolved.ndk, "--android_sdk=" .. resolved.sdk,
            "--ndk_sdkver=26", "--build_toolver=" .. resolved.build_version, "--runtimes=c++_static",
            "--build_examples=y", "--build_tests=n", "--ccache=n", "--android_keystore=" .. key,
            "-o", path.join(root, "build/android"), "-y"}, {envs = envs})
        local args = {"build", "-P", root, "-j", tostring(jobs), "-y"}
        if opt.rebuild then table.insert(args, "-r") end
        if game ~= "all" then table.insert(args, game) end
        os.execv(os.programfile(), args, {envs = envs})
    end
    verify.main(game == "all" and games or {game}, mode, resolved)
end
