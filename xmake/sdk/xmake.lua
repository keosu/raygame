-- Installed in <sdk>/share/raygame; this file only uses the installed SDK.
local sdk_root = path.absolute("../..", os.scriptdir())

add_requires("raylib-cpp 5.5.0")
add_requireconfs("raylib-cpp.raylib", {version = "5.5"})

rule("raygame.sdk")
    on_load(function (target)
        local sdk = assert(io.load(path.join(sdk_root, "share/raygame/sdk.lua")), "Missing RayGame SDK metadata")
        if sdk.syslinks then target:add("syslinks", table.unpack(table.wrap(sdk.syslinks))) end
        if sdk.frameworks then target:add("frameworks", table.unpack(table.wrap(sdk.frameworks))) end
    end)
    on_config(function (target)
        local sdk = assert(io.load(path.join(sdk_root, "share/raygame/sdk.lua")), "Missing RayGame SDK metadata")
        assert(target:plat() == sdk.plat and target:arch() == sdk.arch,
               "RayGame SDK platform/architecture mismatch; install an SDK for this target")
        assert(get_config("mode") == sdk.mode,
               "RayGame SDK mode mismatch; select the matching Debug/Release installation")
        if target:is_plat("windows") then
            local actual = table.concat(table.wrap(target:runtimes()), ",")
            local expected = table.concat(table.wrap(sdk.runtimes), ",")
            assert(actual == expected, "RayGame SDK C runtime mismatch: expected " .. expected .. ", got " .. actual)
        end
    end)
rule_end()

-- Call inside a consumer target after including this file.
function add_raygame_sdk()
    add_rules("raygame.sdk")
    add_includedirs(path.join(sdk_root, "include"))
    add_linkdirs(path.join(sdk_root, "lib"))
    add_links("raygame")
    add_packages("raylib-cpp")
end
