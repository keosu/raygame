-- Extend xmake 3.1.1 NativeActivity packaging with the engine-owned Java bridge.
-- Paths are relative to this file so projects that include raygame (e.g. as a
-- submodule) package the same bridge.
local rule_file = path.join(os.scriptdir(), "android-tts.lua")
local bridge_source = path.join(os.scriptdir(), "../../platform/android/java/org/raygame/SpeechBridge.java")
rule("raygame.android_tts")
    add_deps("android.native_app")
    add_orders("android.native_app", "raygame.android_tts")
    after_build(function (target)
        import("core.project.depend")
        import("lib.detect.find_tool")
        local conf = target:extraconf("rules", "android.native_app")
        local source = path.normalize(bridge_source)
        local sdk = target:toolchain("ndk"):config("android_sdk")
        local version = target:toolchain("ndk"):config("build_toolver")
        local sdktools = path.join(sdk, "build-tools", version)
        local androidjar = path.join(sdk, "platforms", "android-" .. conf.android_sdk_version, "android.jar")
        local java = assert(find_tool("java"), "Java is required to package Android TTS")
        local javac = assert(find_tool("javac"), "javac is required to package Android TTS")
        local base = path.join(target:autogendir(), "packing/res_only.apk")
        local output = path.join(target:targetdir(), target:basename() .. ".apk")
        local work = path.join(target:autogendir(), "speech")
        local keystore = path.absolute(conf.keystore, target:scriptdir())
        assert(os.isfile(base), "NativeActivity resources must be packaged before the TTS bridge")
        depend.on_changed(function ()
            -- This target exclusively owns these generated compiler outputs.
            -- Clear individual outputs so removed anonymous classes cannot survive.
            for _, file in ipairs(os.files(path.join(work, "classes/**.class"))) do os.rm(file) end
            for _, file in ipairs(os.files(path.join(work, "dex/*.dex"))) do os.rm(file) end
            os.mkdir(path.join(work, "classes"))
            os.mkdir(path.join(work, "dex"))
            os.vrunv(javac.program, {"--release", "8", "-encoding", "UTF-8", "-classpath", androidjar,
                                   "-d", path.join(work, "classes"), source})
            local args = {"-cp", path.join(sdktools, "lib/d8.jar"), "com.android.tools.r8.D8",
                          "--min-api", "26", "--lib", androidjar, "--output", path.join(work, "dex")}
            table.join2(args, os.files(path.join(work, "classes/**.class")))
            os.vrunv(java.program, args)
            local stage = path.join(work, "dex")
            os.cp(base, path.join(stage, "speech.apk"))
            local exe = is_host("windows") and ".exe" or ""
            os.vrunv(path.join(sdktools, "aapt" .. exe), {"add", "speech.apk", "classes.dex"}, {curdir = stage})
            os.vrunv(path.join(sdktools, "zipalign" .. exe),
                     {"-f", "-P", "16", "4", "speech.apk", "aligned.apk"}, {curdir = stage})
            os.vrunv(java.program, {"-jar", path.join(sdktools, "lib/apksigner.jar"), "sign",
                "--ks", keystore, "--ks-pass", "pass:" .. conf.keystore_pass,
                "--out", path.absolute(output), path.join(stage, "aligned.apk")})
            cprint("${color.build.target}packaged Android TTS bridge: %s", target:name())
        end, {dependfile = target:dependfile(output .. ".tts"),
              files = {base, source, androidjar, keystore, rule_file},
              values = {version, conf.android_sdk_version}, changed = not os.isfile(output)})
        -- Publish only the completed APK. Native libraries, intermediate APKs and
        -- apksigner's optional v4 sidecars remain in build/.
        -- External consumers keep control of their output layout. Bundled examples
        -- opt in to publishing a copy; using the TTS bridge alone does not do so.
        local published = target:values("raygame.apk_publishdir")
        if published then
            os.mkdir(published)
            local apk = path.join(published, target:basename() .. ".apk")
            if not os.isfile(apk) or io.readfile(apk, {encoding = "binary"}) ~= io.readfile(output, {encoding = "binary"}) then
                os.cp(output, apk)
            end
        end
    end)
rule_end()
