-- Compile/link in build/ so PDB, ILK, archives and dependency state stay there.
-- bin/ contains only runnable games, their assets, and signed APKs.
rule("raygame.publish")
    after_build(function (target)
        import("core.base.option")
        if option.get("dry-run") then return end
        local destination = target:values("raygame.publishdir")
        assert(destination, "Missing game publication directory")
        os.mkdir(destination)
        local output = path.join(destination, path.filename(target:targetfile()))
        -- Compare contents: switching back to an older mode must republish it too.
        if not os.isfile(output) or io.readfile(output, {encoding = "binary"}) ~=
                                   io.readfile(target:targetfile(), {encoding = "binary"}) then
            os.cp(target:targetfile(), output)
        end
    end)
rule_end()
