-- Shared by the repository build and the standalone installed-SDK project.
local forge_root = os.scriptdir()

local function sync_files(source, destination, os, io, assert)
    assert(os.isdir(source), "Missing FORGE input directory: " .. source)
    local files = {}
    for _, file in ipairs(os.files(path.join(source, "**"))) do
        local relative = path.relative(file, source)
        local output = path.join(destination, relative)
        files[relative] = true
        if not os.isfile(output) or io.readfile(file, {encoding = "binary"}) ~= io.readfile(output, {encoding = "binary"}) then
            os.mkdir(path.directory(output))
            os.cp(file, output)
        end
    end
    -- Each destination belongs exclusively to FORGE, including removed assets.
    for _, file in ipairs(os.files(path.join(destination, "**"))) do
        if not files[path.relative(file, destination)] then os.rm(file) end
    end
end

function add_forge_targets(options)
    local build_root = path.absolute(get_config("builddir") or "build", os.projectdir())
    local headers = path.join(build_root, "forge/include")
    if not options.sdk then
        -- Keep the same <raygame/...> includes in both build entry points.
        target("forge_headers")
            set_kind("phony")
            set_default(false)
            set_policy("build.fence", true)
            add_includedirs(headers, {public = true})
            on_build(function ()
                sync_files(path.join(os.projectdir(), "include"), path.join(headers, "raygame"), os, io, assert)
            end)
        target_end()
    end

    local function add_engine()
        if options.sdk then
            add_raygame_sdk()
        else
            add_deps("raygame", "forge_headers")
            set_targetdir("$(projectdir)/bin/forge_runner")
        end
    end

    if options.game then
        target("forge_runner")
            set_kind("binary")
            set_languages("c++17")
            add_files(path.join(forge_root, "src/*.cpp"))
            add_engine()
            if is_plat("windows", "mingw") then
                add_files(path.join(forge_root, "resources/forge.rc"))
                add_includedirs(path.join(forge_root, "resources"))
            end
            if is_plat("windows") then
                add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", {force = true})
            elseif is_plat("mingw") then
                add_ldflags("-mwindows", {force = true})
            end
            after_build(function (target)
                sync_files(path.join(forge_root, "assets"), path.join(target:targetdir(), "assets"), os, io, assert)
                for _, name in ipairs({"README.md", "THIRD_PARTY_NOTICES.md"}) do
                    local source = path.join(forge_root, name)
                    local destination = path.join(target:targetdir(), name)
                    if not os.isfile(destination) or io.readfile(source) ~= io.readfile(destination) then
                        os.cp(source, destination)
                    end
                end
            end)
        target_end()
    end

    if options.tests then
        target("forge_tests")
            set_kind("binary")
            set_languages("c++17")
            set_default(false)
            add_files(path.join(forge_root, "tests/main.cpp"), path.join(forge_root, "src/World.cpp"),
                      path.join(forge_root, "src/Profile.cpp"), path.join(forge_root, "src/Localization.cpp"))
            add_engine()
            set_runargs(path.join(forge_root, "assets"), path.join(build_root, "forge-tests/$(plat)/$(arch)/$(mode)"))
        target_end()
    end
end
