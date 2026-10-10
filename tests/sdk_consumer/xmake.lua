set_project("raygame-sdk-consumer")
set_languages("c++17")
set_encodings("utf-8")
add_rules("mode.debug", "mode.release")

option("raygame_sdk")
    set_showmenu(true)
    set_description("Path to an installed RayGame SDK")
option_end()

local sdk = get_config("raygame_sdk")
if sdk then
    includes(path.join(sdk, "share/raygame/xmake.lua"))
    target("sdk_consumer")
        set_kind("binary")
        add_files("main.cpp", "individual_headers.cpp")
        add_raygame_sdk()
    target_end()
end
