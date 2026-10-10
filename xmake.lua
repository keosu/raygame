set_project("raygame")
set_version("0.2.0")

add_rules("mode.debug", "mode.release")
set_encodings("utf-8")

if is_plat("android") then includes("xmake/packages/raylib.lua") end
add_requires("raylib-cpp 5.5.0")
add_requireconfs("raylib-cpp.raylib", {version = "5.5"})

if is_plat("android") then
    set_runtimes("c++_static")
    add_defines("PLATFORM_ANDROID", "GRAPHICS_API_OPENGL_ES2")
end

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

includes("examples/xmake.lua", "tests/xmake.lua")
