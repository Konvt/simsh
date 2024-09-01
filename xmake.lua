add_rules("mode.debug", "mode.release")

set_languages("c++20")

add_cxxflags("-Wall", "-Wpedantic", "-Wextra", "-Wshadow")

target("simsh")
  set_kind("binary")
  add_files("src/*.cpp", "util/*.cpp")
  add_includedirs("src/", "util/")
  set_targetdir(os.curdir())

target("format")
  set_kind("phony")
  on_run(function (target)
    for _, filepath in ipairs(os.files("src/*.*pp")) do
      os.run(format("clang-format -i %s", filepath))
    end

    for _, filepath in ipairs(os.files("util/*.*pp")) do
      os.run(format("clang-format -i %s", filepath))
    end
  end)
