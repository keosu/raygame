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

local example_names = {"space_shooter", "bouncing_balls", "neon_breakout", "bullet_hell", "engine_lab", "marble_temple", "garden_guard"}
local examples_root = os.scriptdir()

-- Each game owns its assets; only the small examples share fonts.
-- Keep historical desktop EXE paths and stage APK assets under the build directory.
-- Pass runtime APIs from the build hook; the declaration scope has restricted I/O.
local function copy_assets(source, destination, os, io)
    local copied = {}
    os.mkdir(destination)
    for _, file in ipairs(os.files(path.join(source, "**"))) do
        local relative = path.relative(file, source)
        local output = path.join(destination, relative)
        copied[relative] = true
        if not os.isfile(output) or io.readfile(file, {encoding = "binary"}) ~= io.readfile(output, {encoding = "binary"}) then
            os.mkdir(path.directory(output))
            os.cp(file, output)
        end
    end
    return copied
end

if not is_plat("android") and (has_config("build_examples") or has_config("build_tests")) then
    target("example_assets")
        set_kind("phony")
        set_default(false)
        on_build(function (target)
            copy_assets(path.join(examples_root, "common/assets"), path.join(os.projectdir(), "bin/assets"), os, io)
        end)
    target_end()
    for _, name in ipairs(example_names) do
        target(name .. "_assets")
            set_kind("phony")
            set_default(false)
            on_build(function ()
                copy_assets(path.join(examples_root, name, "assets"), path.join(os.projectdir(), "bin/assets", name), os, io)
                if name == "engine_lab" or name == "marble_temple" or name == "garden_guard" then
                    os.cp(path.join(examples_root, name, "README.md"), path.join(os.projectdir(), "bin", name .. "-README.md"))
                end
            end)
        target_end()
    end
end

local function add_example(name)
    target(name)
        set_kind("binary")
        set_languages("c++17")
        set_basename(name)
        add_files(name .. "/main.cpp")
        add_deps("raygame")
        if is_plat("android") then
            local staged_assets = path.absolute(path.join(get_config("builddir"), "example-assets", name), os.projectdir())
            set_kind("shared")
            set_targetdir("$(projectdir)/bin/android/$(arch)/$(mode)")
            -- The Android rule resolves resource paths relative to this script.
            add_rules("android.native_app", {
                android_sdk_version = "35",
                android_manifest = name .. "/android/AndroidManifest.xml",
                android_res = name .. "/android/res",
                android_assets = staged_assets,
                keystore = path.absolute(get_config("android_keystore"), os.projectdir()),
                keystore_pass = "android",
                package_name = "org.raygame." .. name:gsub("_", ""),
                native_app_glue = false,
                logcat_filters = {"raygame", "raylib"}
            })
            add_rules("raygame.android_tts")
            before_build(function ()
                local shared = copy_assets(path.join(examples_root, "common/assets"), staged_assets, os, io)
                local owned = copy_assets(path.join(examples_root, name, "assets"), staged_assets, os, io)
                -- This target owns the staging directory. Drop removed assets so
                -- incremental APK builds cannot retain another game's resources.
                for _, file in ipairs(os.files(path.join(staged_assets, "**"))) do
                    local relative = path.relative(file, staged_assets)
                    if not shared[relative] and not owned[relative] then os.rm(file) end
                end
            end)
            add_shflags("-Wl,-u,ANativeActivity_onCreate", "-Wl,-u,__wrap_android_main",
                        "-Wl,--wrap=android_main", "-Wl,-z,max-page-size=16384",
                        "-Wl,--no-undefined", {force = true})
        else
            add_deps("example_assets", name .. "_assets")
            set_targetdir("$(projectdir)/bin")
            if is_plat("windows", "mingw") then
                add_files(name .. "/resources/windows/" .. name .. ".rc")
                add_includedirs(name .. "/resources/windows")
            end
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

if has_config("build_examples") then
    for _, name in ipairs(example_names) do add_example(name) end
end

if has_config("build_examples") or has_config("build_tests") then
    includes("forge_runner/targets.lua")
    add_forge_targets({game = has_config("build_examples"), tests = has_config("build_tests")})
end
