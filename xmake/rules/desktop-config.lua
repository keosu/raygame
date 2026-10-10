-- Desktop EXEs keep their historical bin paths. xmake's link cache is per mode,
-- so also record which configuration actually produced the shared output file.
local function configuration(target)
    return table.concat({target:plat(), target:arch(), get_config("mode") or "",
                         table.concat(table.wrap(target:runtimes()), ","),
                         target:tool("cxx") or "", tostring(get_config("example_console"))}, "\n")
end

rule("raygame.desktop_config")
    before_link(function (target)
        import("core.base.option")
        if not target:is_binary() or target:is_plat("android") or option.get("dry-run") then return end
        local marker = target:targetfile() .. ".raygame-config"
        local previous = os.isfile(marker) and io.readfile(marker) or ""
        if previous ~= configuration(target) then
            -- Force only the link step; existing object files remain reusable.
            if os.isfile(target:dependfile()) then os.rm(target:dependfile()) end
        end
    end)
    after_link(function (target)
        import("core.base.option")
        if not target:is_binary() or target:is_plat("android") or option.get("dry-run") then return end
        io.writefile(target:targetfile() .. ".raygame-config", configuration(target), {only_changed = true})
    end)
rule_end()
