task("android")
    set_category("plugin")
    on_run(function ()
        import("core.base.option")
        import("scripts.android.build", {rootdir = os.projectdir()}).main(option.options())
    end)
    set_menu {
        usage = "xmake android [options]",
        description = "Build and verify Android APKs without changing the desktop configuration.",
        options = {
            {"m", "mode", "kv", "release", "Build mode: debug or release."},
            {"g", "game", "kv", "all", "Example target, or all eight games."},
            {"j", "jobs", "kv", "4", "Parallel compiler jobs."},
            {nil, "sdk", "kv", nil, "Android SDK directory."},
            {nil, "ndk", "kv", nil, "Android NDK directory (auto-detected by default)."},
            {nil, "jdk", "kv", nil, "JDK 17+ directory."},
            {nil, "build-tools", "kv", nil, "Installed Build Tools version (>=35)."},
            {nil, "verify-only", "k", nil, "Verify existing APKs without configuring/building."},
            {nil, "rebuild", "k", nil, "Rebuild native code without the compiler cache."}
        }
    }
task_end()

task("android-setup")
    set_category("plugin")
    on_run(function ()
        import("core.base.option")
        import("scripts.android.setup", {rootdir = os.projectdir()}).main(option.options())
    end)
    set_menu {
        usage = "xmake android-setup [options]",
        description = "Install missing Android tools under build/android/tools using Google's SDK manager.",
        options = {
            {nil, "sdk", "kv", nil, "SDK to complete; defaults to build/android/tools/sdk."},
            {nil, "jdk", "kv", nil, "Existing JDK 17+ (download one if absent)."},
            {nil, "accept-licenses", "k", nil, "Accept Android SDK licenses; otherwise sdkmanager asks interactively."}
        }
    }
task_end()
