-- Run after building raygame: xmake lua scripts/verify-sdk.lua [--graphics]
import("core.project.config")

function main(graphics)
    assert(not graphics or graphics == "--graphics", "Usage: verify-sdk.lua [--graphics]")
    assert(config.load(), "Configure and build RayGame before verifying the SDK")
    local root = os.projectdir()
    local mode = config.get("mode")
    local plat = config.get("plat")
    assert(plat ~= "android", "The SDK consumer check currently targets desktop builds")
    local logs = path.join(root, "build/sdk-validation", plat, config.get("arch"), mode)
    os.mkdir(logs)
    -- A failed rerun must never leave an old passing report behind.
    os.tryrm(path.join(logs, "report.lua"))
    -- Fresh, unrelated directories also test paths containing spaces.
    local workspace = os.tmpfile() .. "-raygame sdk"
    local staged = path.join(workspace, "staged")
    local installed = path.join(workspace, "relocated SDK")
    local consumer = path.join(workspace, "consumer")
    os.mkdir(consumer)
    for _, name in ipairs({"xmake.lua", "main.cpp", "individual_headers.cpp"}) do
        os.cp(path.join(root, "tests/sdk_consumer", name), consumer)
    end

    local function run(name, args, directory)
        print("SDK check: %s", name)
        local logfile = path.join(logs, name .. ".log")
        local status = os.execv(os.programfile(), args, {
            curdir = directory, stdout = logfile, stderr = logfile, try = true
        })
        assert(status == 0, "SDK check failed: " .. name .. "; see " .. logfile)
        return io.readfile(logfile)
    end
    run("install", {"install", "-o", staged, "raygame"}, root)
    -- Move the installed tree so no consumer can rely on its original prefix.
    -- Both paths are literal children of the fresh workspace created above.
    assert(path.directory(path.absolute(staged)) == path.absolute(workspace))
    assert(path.directory(path.absolute(installed)) == path.absolute(workspace))
    os.mv(staged, installed)
    assert(#os.files(path.join(installed, "include/*.h")) == 0, "SDK headers must be namespaced")
    for _, header in ipairs(os.files(path.join(root, "include/*.h"))) do
        local copy = path.join(installed, "include/raygame", path.filename(header))
        assert(os.isfile(copy) and io.readfile(copy) == io.readfile(header), "Installed header differs: " .. header)
    end
    local metadata = io.load(path.join(installed, "share/raygame/sdk.lua"))
    assert(metadata and metadata.mode == mode and metadata.plat == plat)
    local sdkfile = path.join(installed, "share/raygame/sdk.lua")
    local args = {"f", "-p", metadata.plat, "-a", metadata.arch, "-m", metadata.mode,
                  "--raygame_sdk=" .. installed, "-y"}
    if plat == "windows" then table.insert(args, "--toolchain=msvc") end

    run("configure", args, consumer)
    run("build", {"build", "-v", "-y"}, consumer)
    print(run("run", {"run", "sdk_consumer"}, consumer):trim())
    if graphics then
        run("graphics", {"run", "sdk_consumer", "--graphics"}, consumer)
    end

    -- Corrupt only this disposable SDK's declared configuration. The consumer
    -- must reject each mismatch, even if all required libraries are present.
    local function reject(field, wrong, expected)
        local original = metadata[field]
        metadata[field] = wrong
        io.save(sdkfile, metadata)
        local output = path.join(logs, "reject-" .. field .. ".log")
        local status = os.execv(os.programfile(), args, {
            curdir = consumer, stdout = output, stderr = output, try = true
        })
        metadata[field] = original
        io.save(sdkfile, metadata)
        assert(status and status ~= 0, "SDK mismatch unexpectedly accepted: " .. field)
        assert(io.readfile(output):find(expected, 1, true), "Unexpected failure while checking " .. field)
        print("SDK check: rejected %s mismatch", field)
    end
    reject("mode", mode == "debug" and "release" or "debug", "RayGame SDK mode mismatch")
    reject("arch", "invalid-sdk-architecture", "RayGame SDK platform/architecture mismatch")
    if plat == "windows" then
        reject("runtimes", metadata.runtimes == "MD" and "MT" or "MD", "RayGame SDK C runtime mismatch")
    end
    run("restore-configure", args, consumer)
    print(run("restore-run", {"run", "sdk_consumer"}, consumer):trim())
    io.save(path.join(logs, "report.lua"), {
        sdk = metadata, workspace = workspace, consumer = consumer,
        installed = installed, passed = true, graphics = graphics ~= nil
    })
    print("SDK verification passed; logs: %s", logs)
end
