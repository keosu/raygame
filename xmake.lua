set_project("raygame")
set_version("0.2.0")

add_rules("mode.debug", "mode.release")
set_encodings("utf-8")

if is_plat("android") then includes("android/raylib.lua") end
add_requires("raylib-cpp 5.5.0")
add_requireconfs("raylib-cpp.raylib", {version = "5.5"})

if is_plat("android") then
    set_runtimes("c++_static")
    add_defines("PLATFORM_ANDROID", "GRAPHICS_API_OPENGL_ES2")
end

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

-- RayGame Engine Library (static)
target("raygame")
    set_kind("static")
    set_languages("c++17")

    add_files("src/*.cpp")
    add_headerfiles("include/*.h")
    add_includedirs("include", {public = true})

    add_packages("raylib-cpp", {public = true})

    set_targetdir(is_plat("android") and "$(projectdir)/bin/android/$(arch)/$(mode)" or "$(projectdir)/bin")

    if is_plat("windows") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", "shell32", "ole32", {public = true})
    elseif is_plat("mingw") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", "shell32", "ole32", {public = true})
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl", "rt", {public = true})
    elseif is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreAudio", {public = true})
    elseif is_plat("android") then
        add_syslinks("log", "android", "EGL", "GLESv2", "OpenSLES", "m", "dl", {public = true})
        on_load(function (target)
            local ndk = assert(target:toolchain("ndk"):config("ndk"), "Android NDK is required")
            target:add("includedirs", path.join(ndk, "sources/android/native_app_glue"))
        end)
    end

    -- Install headers + library
    add_installfiles("include/(*.h)", {prefixdir = "include/raygame"})
target_end()

-- Helper to declare an example target
target("example_assets")
    set_kind("phony")
    set_default(false)
    on_build(function (target)
        os.mkdir(path.join(os.projectdir(), "bin"))
        os.cp(path.join(os.projectdir(), "assets"), path.join(os.projectdir(), "bin"))
    end)
target_end()

function add_example(name, dir)
    target(name)
        set_kind("binary")
        set_languages("c++17")
        set_basename(name)
        add_files("examples/" .. dir .. "/main.cpp")
        add_deps("raygame")
        if is_plat("android") then
            set_kind("shared")
            set_targetdir("$(projectdir)/bin/android/$(arch)/$(mode)")
            add_rules("android.native_app", {
                android_sdk_version = "35",
                android_manifest = "android/" .. name .. "/AndroidManifest.xml",
                android_assets = "assets",
                keystore = get_config("android_keystore"),
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

add_example("space_shooter", "space_shooter")
add_example("bouncing_balls", "bouncing_balls")
add_example("neon_breakout", "neon_breakout")
add_example("bullet_hell", "bullet_hell")

-- Unit tests (logic-only, no window required)
if not is_plat("android") then
target("tests")
    set_kind("binary")
    set_languages("c++17")
    set_basename("tests")
    set_default(false)
    add_files("tests/main.cpp", "tests/header_compat.cpp", "tests/platform_files.cpp")
    add_deps("raygame")
    set_targetdir("$(projectdir)/bin")
target_end()

-- Optional graphics integration test (requires an OpenGL-capable display).
target("smoke")
    set_kind("binary")
    set_languages("c++17")
    set_default(false)
    add_files("tests/smoke.cpp", "tests/window_theme.cpp")
    add_deps("raygame", "example_assets")
    set_targetdir("$(projectdir)/bin")
target_end()
end
