-- Local Android recipe for raylib 5.5. The upstream xmake recipe adds
-- --wrap=fopen, but 5.5 already maps file IO to android_fopen in utils.c.
-- Wrapping it again leaves __wrap_fopen undefined in the package link check.
package("raylib")
    set_homepage("https://www.raylib.com")
    set_description("raylib 5.5 Android backend without the redundant fopen wrapper")
    set_license("zlib")
    add_urls("https://github.com/raysan5/raylib/archive/$(version).tar.gz")
    add_versions("5.5", "aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e")
    add_deps("cmake >=3.11")
    add_syslinks("log", "android", "EGL", "GLESv2", "OpenSLES", "m", "dl")
    on_install("android", function (package)
        import("package.tools.cmake").install(package, {
            "-DPLATFORM=Android", "-DOPENGL_VERSION=ES 2.0", "-DANDROID_ABI=" .. package:arch(),
            "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"),
            "-DBUILD_SHARED_LIBS=OFF", "-DBUILD_EXAMPLES=OFF", "-DWITH_PIC=ON"
        })
    end)
    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test() { InitWindow(100, 100, "raygame link check"); }
        ]]}, {includes = "raylib.h", configs = {languages = "c++17"}}))
    end)
package_end()
