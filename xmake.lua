-- raygame can also be included from another project's xmake.lua (e.g. as a git submodule):
-- it then only contributes the engine target, keeps that project's name, and leaves the
-- bundled examples and tests off unless requested with --build_examples/--build_tests.
local standalone = path.absolute(os.projectdir()) == path.absolute(os.scriptdir())
if standalone then
    set_project("raygame")
    set_version("0.2.0")
end

includes("xmake/rules/publish.lua")
includes("xmake/rules/android-tts.lua")
if standalone then includes("xmake/tasks/android.lua") end
add_rules("mode.debug", "mode.release")
set_encodings("utf-8")

option("build_examples")
    set_default(standalone)
    set_showmenu(true)
    set_description("Build the bundled example games")
option_end()

option("build_tests")
    set_default(standalone)
    set_showmenu(true)
    set_description("Define the desktop logic and graphics smoke targets")
option_end()

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
    add_headerfiles("include/(*.h)", {prefixdir = "raygame"})
    add_includedirs("include", {public = true})

    add_packages("raylib-cpp", {public = true})

    -- Libraries must survive mode switches without reusing another mode's archive.
    set_targetdir("$(builddir)/lib/$(plat)/$(arch)/$(mode)")

    if is_plat("windows") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", "shell32", "ole32", "sapi", {public = true})
    elseif is_plat("mingw") then
        add_syslinks("winmm", "gdi32", "dwmapi", "advapi32", "shell32", "ole32", "sapi", {public = true})
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl", "rt", {public = true})
    elseif is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreAudio", {public = true})
    elseif is_plat("android") then
        -- This archive is linked into each NativeActivity shared library.
        -- Debug retains global/TLS references that also require PIC on ARM64.
        add_cxflags("-fPIC", {force = true})
        add_syslinks("log", "android", "EGL", "GLESv2", "OpenSLES", "m", "dl", {public = true})
        on_load(function (target)
            local ndk = assert(target:toolchain("ndk"):config("ndk"), "Android NDK is required")
            target:add("includedirs", path.join(ndk, "sources/android/native_app_glue"))
        end)
    end

    add_installfiles("xmake/sdk/xmake.lua", {prefixdir = "share/raygame"})
    after_install(function (target)
        -- No source/package-cache paths are written into the relocatable SDK.
        io.save(path.join(target:installdir(), "share/raygame/sdk.lua"), {
            version = target:version(),
            plat = target:plat(),
            arch = target:arch(),
            mode = get_config("mode"),
            runtimes = target:runtimes(),
            syslinks = target:get("syslinks"),
            frameworks = target:get("frameworks")
        })
    end)
target_end()

includes("examples/xmake.lua", "tests/xmake.lua")
