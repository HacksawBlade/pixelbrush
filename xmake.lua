add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")
set_encodings("utf-8")
add_includedirs("include")
add_includedirs("vendor/libargon")
set_warnings("all")

target("pixelbrush")
    set_kind("binary")
    add_files("src/*.c", {languages = "c11"})
    add_files("src/*.cpp")
    add_syslinks("WindowsApp", "Ole32", "windowscodecs")
    set_default(true)

target("gen_tty_maps")
    set_kind("binary")
    add_files("tools/gen_tty_maps.cpp")

task("build-all")
    set_menu {
        usage = "xmake build-all",
        description = "Build release binaries for all architectures (x64, x86, arm64)",
    }
    on_run(function ()
        for _, arch in ipairs({"x64", "x86", "arm64"}) do
            print("=== Building " .. arch .. " ===")
            os.exec("xmake f -a %s -m release -y", arch)
            os.exec("xmake build")
        end
    end)
