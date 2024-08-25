add_rules("mode.debug", "mode.release")

target("gobang-server")
    set_kind("binary")
    add_includedirs(
        ".",
        "lib/openjson"
    )
    add_files(
        "./*cpp",
        "lib/openjson/*.cpp"
    )
