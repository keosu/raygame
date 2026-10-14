-- Extend xmake 3.1.1 NativeActivity packaging with the engine-owned Java bridge.
rule("raygame.android_tts")
    add_deps("android.native_app")
    add_orders("android.native_app", "raygame.android_tts")
    after_build(function (target)
        import("core.project.depend")
        import("lib.detect.find_tool")
        local conf = target:extraconf("rules", "android.native_app")
        local source = path.join(os.projectdir(), "platform/android/java/org/raygame/SpeechBridge.java")
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
              files = {base, source, androidjar, keystore, path.join(os.projectdir(), "xmake/rules/android-tts.lua")},
              values = {version, conf.android_sdk_version}, changed = not os.isfile(output)})
        -- Publish only the completed APK. Native libraries, intermediate APKs and
        -- apksigner's optional v4 sidecars remain in build/.
        local published = path.join(os.projectdir(), "bin/android", target:arch(), get_config("mode"))
        os.mkdir(published)
        local apk = path.join(published, target:basename() .. ".apk")
        if not os.isfile(apk) or io.readfile(apk, {encoding = "binary"}) ~= io.readfile(output, {encoding = "binary"}) then
            os.cp(output, apk)
        end
    end)
rule_end()
