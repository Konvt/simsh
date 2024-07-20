add_rules("mode.debug", "mode.release")

set_languages("c++20")

add_cxxflags("-Wall", "-Wpedantic", "-Wextra", "-Wshadow")

target("simsh")
  set_kind("binary")
  add_files("src/*.cpp", "util/*.cpp")
  add_includedirs("src/", "util/")
  set_targetdir(os.curdir())
