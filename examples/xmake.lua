option("android_keystore")
    set_default("build/android/debug.keystore")
    set_showmenu(true)
    set_description("Debug keystore for local Android APK builds (password: android)")
option_end()

option("example_console")
    set_default(false)
    set_showmenu(true)
    set_description("Enable the Windows console and raylib logs for examples")
option_end()

-- Shared desktop resources for the games and the graphics smoke test.
target("example_assets")
    set_kind("phony")
    set_default(false)
    on_build(function (target)
        local destination = path.join(os.projectdir(), "bin")
        os.mkdir(destination)
        os.cp(path.join(target:scriptdir(), "assets"), destination)
    end)
target_end()

local function add_example(name)
    target(name)
        set_kind("binary")
        set_languages("c++17")
        set_basename(name)
        add_files(name .. "/main.cpp")
        add_deps("raygame")
        if is_plat("android") then
            set_kind("shared")
            set_targetdir("$(projectdir)/bin/android/$(arch)/$(mode)")
            -- The Android rule resolves resource paths relative to this script.
            add_rules("android.native_app", {
                android_sdk_version = "35",
                android_manifest = "android/" .. name .. "/AndroidManifest.xml",
                android_res = "android/" .. name .. "/res",
                android_assets = "assets",
                keystore = path.absolute(get_config("android_keystore"), os.projectdir()),
                keystore_pass = "android",
                package_name = "org.raygame." .. name:gsub("_", ""),
                native_app_glue = false,
                logcat_filters = {"raygame", "raylib"}
            })
            add_shflags("-Wl,-u,ANativeActivity_onCreate", "-Wl,-u,__wrap_android_main",
                        "-Wl,--wrap=android_main", "-Wl,-z,max-page-size=16384",
                        "-Wl,--no-undefined", {force = true})
        else
            add_deps("example_assets")
            set_targetdir("$(projectdir)/bin")
            if is_plat("windows", "mingw") then
                add_files("resources/windows/" .. name .. ".rc")
                add_includedirs("resources/windows")
            end
        end
        if not has_config("example_console") then
            add_defines("RAYGAME_QUIET_EXAMPLE")
            if is_plat("windows") then
                -- Keep the standard main() and CRT startup in a GUI executable.
                add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", {force = true})
            elseif is_plat("mingw") then
                add_ldflags("-mwindows", {force = true})
            end
        end
    target_end()
end

add_example("space_shooter")
add_example("bouncing_balls")
add_example("neon_breakout")
add_example("bullet_hell")
