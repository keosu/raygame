add_rules("mode.debug", "mode.release")

add_requires("raylib-cpp 5.5.0")

-- RayGame Engine Library
target("raygame")
    set_kind("static")
    set_languages("c++17")
    
    -- Define export macro
    add_defines("RAYGAME_EXPORTS")
    
    -- Source files
    add_files("src/*.cpp")
    add_headerfiles("include/*.h")
    add_includedirs("include", {public = true})
    
    -- Dependencies (public so they propagate to dependent targets)
    add_packages("raylib-cpp", {public = true})
    
    -- Output directory
    set_targetdir("$(projectdir)/bin")
    
    -- Platform-specific settings (public so they propagate to dependent targets)
    if is_plat("windows") then
        add_syslinks("winmm", "gdi32", {public = true})
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl", "rt", {public = true})
    elseif is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreAudio", {public = true})
    end

-- Space Shooter Example
target("space_shooter")
    set_kind("binary")
    set_languages("c++17")
    set_basename("space_shooter")
    
    add_files("examples/space_shooter/main.cpp")
    add_deps("raygame")
    
    -- Set output directory
    set_targetdir("$(projectdir)/bin")
    
    -- Copy assets after build (if they exist)
    after_build(function (target)
        if os.isdir("assets") then
            os.cp("assets", path.join(target:targetdir(), "assets"))
        end
    end)

-- Bouncing Balls Example
target("bouncing_balls")
    set_kind("binary")
    set_languages("c++17")
    set_basename("bouncing_balls")
    
    add_files("examples/bouncing_balls/main.cpp")
    add_deps("raygame")
    
    -- Set output directory
    set_targetdir("$(projectdir)/bin")
    
    -- Copy assets after build (if they exist)
    after_build(function (target)
        if os.isdir("assets") then
            os.cp("assets", path.join(target:targetdir(), "assets"))
        end
    end)
