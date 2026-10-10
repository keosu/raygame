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
    includes("targets.lua")
    add_forge_targets({sdk = true, game = true, tests = true})
end
