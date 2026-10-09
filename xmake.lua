set_project("raygame")
set_version("0.2.0")

add_rules("mode.debug", "mode.release")
set_encodings("utf-8")

add_requires("raylib-cpp 5.5.0")

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
        add_syslinks("winmm", "gdi32", {public = true})
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl", "rt", {public = true})
    elseif is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreAudio", {public = true})
    end

    -- Install headers + library
    add_installfiles("include/(*.h)", {prefixdir = "include/raygame"})
target_end()

-- Helper to declare an example target
function add_example(name, dir)
    target(name)
        set_kind("binary")
        set_languages("c++17")
        set_basename(name)
        add_files("examples/" .. dir .. "/main.cpp")
        add_deps("raygame")
        set_targetdir("$(projectdir)/bin")
        after_build(function (target)
            if os.isdir("assets") then
                os.cp("assets", path.join(target:targetdir(), "assets"))
            end
        end)
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
    add_files("tests/smoke.cpp")
    add_deps("raygame")
    set_targetdir("$(projectdir)/bin")
target_end()
