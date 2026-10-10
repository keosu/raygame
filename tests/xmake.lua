-- Desktop tests; the smoke target also exercises the real example games.
if has_config("build_tests") and not is_plat("android") then
    target("tests")
        set_kind("binary")
        set_languages("c++17")
        set_basename("tests")
        set_default(false)
        add_files("main.cpp", "header_compat.cpp", "platform_files.cpp")
        add_deps("raygame")
        set_targetdir("$(projectdir)/bin")
    target_end()

    -- Requires an OpenGL-capable display.
    target("smoke")
        set_kind("binary")
        set_languages("c++17")
        set_default(false)
        add_files("smoke.cpp", "window_theme.cpp")
        add_deps("raygame", "example_assets")
        set_targetdir("$(projectdir)/bin")
    target_end()
end
