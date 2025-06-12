add_rules("mode.debug", "mode.release")

-- 主程序目标
target("PikiniDesktop")
    set_kind("binary")
    add_files("src/main.cpp")  -- 只编译主程序
    add_syslinks("user32")
    set_languages("c++17")
    add_defines("UNICODE", "_UNICODE")

-- 单元测试目标
target("test_algorithm")
    set_kind("binary")
    add_files("test/Algorithm.cpp") 
    set_languages("c++17")