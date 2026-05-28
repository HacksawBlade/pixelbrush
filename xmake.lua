add_rules("mode.debug", "mode.release")
set_languages("c11")
set_encodings("utf-8")
add_includedirs("include")
set_warnings("all")

target("pixelbrush")
    set_kind("binary")
    add_files("src/*.c")
    if is_mode("debug") then
        add_defines("DEBUG")
    else
        add_defines("NDEBUG")
    end
