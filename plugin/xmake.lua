target("kittyhook")
    set_arch("x86")
    set_kind("shared")
    set_languages("c11", "c++23")
    add_defines("UNICODE")
    add_links("user32")
    add_files("src/*.cpp")
	add_files("src/thirdparty/*.cpp")
    add_files("src/thirdparty/*.c")
    add_includedirs("include/")
	add_includedirs("include/thirdparty/")
    after_build(function (target)
        os.cp(target:targetfile(), "./dist/tf/addons/")
    end)
    after_load(function (target)
        if os.getenv("XMAKE_IN_PROJECT_GENERATOR") then
            target:set("toolchains", "clang")
        end
        for _, toolchain in ipairs(target:toolchains()) do
            toolchain:check()
        end
    end)