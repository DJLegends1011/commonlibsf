includes("../..")
set_languages("c++23")
add_rules("mode.debug", "mode.releasedbg")

target("message-box-tests")
    set_kind("binary")
    set_default(true)
    add_deps("commonlibsf")
    add_files("main.cpp")
