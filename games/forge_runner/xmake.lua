set_project("forge-runner")
set_version("1.0.0")
set_languages("c++17")
set_encodings("utf-8")
add_rules("mode.debug", "mode.release")

option("raygame_sdk")
    set_showmenu(true)
    set_description("Installed RayGame SDK directory")
option_end()

local sdk = get_config("raygame_sdk")
if sdk then
    includes(path.join(sdk, "share/raygame/xmake.lua"))
    target("forge_runner")
        set_kind("binary")
        add_files("src/*.cpp", "resources/forge.rc")
        if not is_plat("windows", "mingw") then remove_files("resources/forge.rc") end
        add_includedirs("resources")
        add_raygame_sdk()
        if is_plat("windows") then
            add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", {force = true})
        elseif is_plat("mingw") then
            add_ldflags("-mwindows", {force = true})
        end
        after_build(function (target)
            os.cp("assets", target:targetdir())
        end)
    target_end()

    target("forge_tests")
        set_kind("binary")
        set_default(false)
        add_files("tests/main.cpp", "src/World.cpp", "src/Profile.cpp")
        add_raygame_sdk()
    target_end()
end
