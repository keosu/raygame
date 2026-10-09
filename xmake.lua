set_project("raygame")
set_version("0.2.0")

add_rules("mode.debug", "mode.release")
set_encodings("utf-8")

add_requires("raylib-cpp 5.5.0")

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

    set_targetdir("$(projectdir)/bin")

    if is_plat("windows") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", {public = true})
    elseif is_plat("mingw") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", {public = true})
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl", "rt", {public = true})
    elseif is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreAudio", {public = true})
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
        add_deps("raygame", "example_assets")
        set_targetdir("$(projectdir)/bin")
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
target("tests")
    set_kind("binary")
    set_languages("c++17")
    set_basename("tests")
    set_default(false)
    add_files("tests/main.cpp", "tests/header_compat.cpp")
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
