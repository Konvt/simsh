# tish - A Tiny Shell written in modern C++.

**Contents**
- [tish - A Tiny Shell written in modern C++.](#tish---a-tiny-shell-written-in-modern-c)
  - [Features](#features)
  - [How to build](#how-to-build)
    - [Makefile](#makefile)
    - [CMake](#cmake)
    - [Binary](#binary)
  - [How to use](#how-to-use)
- [tish - 由 Modern C++ 编写的 Tiny Shell - zh\_cn](#tish---由-modern-c-编写的-tiny-shell---zh_cn)
  - [特点](#特点)
  - [如何构建](#如何构建)
    - [Makefile](#makefile-1)
    - [CMake](#cmake-1)
    - [Binary](#binary-1)
  - [如何使用](#如何使用)

A Tiny Shell written in modern C++.

## Features
- Supports nested statements
- Supports most redirection functionalities, including pipes
- Syntax parsing based on recursive descent method
- Modular design
- Adheres to Modern C++ standards

> This is just a *toy project* for personal practice.
> \
> If you are looking for a mature and reliable shell program, this project may not meet your expectations.

## How to build
The syntax standard used is C++20 (with `ranges` and `format`), so the compiler required for building needs to be at least g++13 or clang14.

The project provides two build methods, **but it is recommended not to use both of them simultaneously.**

Each method assumes the compilation tool **fully supports** `std::ranges` or `std::format`.

Run the program with `./tish`.

### Makefile
```sh
make -j release
```
### CMake
```sh
# requires CMake 3.21
cmake --preset release && cmake --build --preset release
```

or

```sh
cmake -S . -B build && cmake --build build
```

### Binary
Since some functions rely on `glibc`, the [binary files](https://github.com/Konvt/tish/releases/tag/v0.1.1) are not guaranteed to run correctly.

For more details, refer to the build environment declarations in [Releases](https://github.com/Konvt/tish/releases).

## How to use
Just like using bash.

You can pass command line arguments at startup to execute a single statement.
```sh
./tish -c "whoami && pwd"  # Single statement must include the -c parameter
./tish "whoami" "&&" "pwd" # Or split into multiple statements in advance, no -c parameter needed
```

You can also read commands from a file.
```sh
(echo "whoami && pwd" > ./script.txt) && ./tish ./script.txt
# No specific extension required, just needs to be a text format file
```

Additionally, `tish` will expand special symbols (i.e., `$$` and `~`) to the current program's `pid` or the user's home directory path, respectively.
```sh
./tish -c "echo ~ && echo \$\$"
```

If run as the `root` user, the default `tish::CLI` object will change the command prompt to a colorless format ending with `#`.

- - -

# tish - 由 Modern C++ 编写的 Tiny Shell - zh_cn

一个由 Modern C++ 编写的、简单的 Linux shell。

## 特点
- 支持嵌套语句
- 支持包括管道在内的大部分重定向功能
- 基于递归下降法进行语法解析
- 模块化设计
- 遵循 Modern C++ 规范

## 如何构建
使用的语法标准为 C++20（with `ranges` and `format`），故构建时的编译器需要至少是 g++13，或 clang14

项目提供了两种构建方式，**但这里建议不要同时使用它们**。

每种方式都假定使用的编译工具**已完全支持** `std::ranges` 或 `std::format`。

使用 `./tish` 运行程序。
### Makefile
```sh
make -j release
```
### CMake
```sh
# 需要 CMake 3.21
cmake --preset release && cmake --build --preset release
```

或者是

```sh
cmake -S . -B build && cmake --build build
```
### Binary
因为部分函数依赖于 `glibc`，因此不保证[二进制文件](https://github.com/Konvt/tish/releases/tag/v0.1.1)能够正常运行。

详情可以参照 [Releases](https://github.com/Konvt/tish/releases) 中的构建环境声明。

## 如何使用
就像使用 bash 一样。

可以在启动时传入命令行参数，以执行单条语句。
```sh
./tish -c "whoami && pwd"  # 单条语句必须带有 -c 参数
./tish "whoami" "&&" "pwd" # 或者提前分割为多条语句传入，此时不需要 -c 参数
```

也可以从文件读取命令。
```sh
(echo "whoami && pwd" > ./script.txt) && ./tish ./script.txt
# 没有后缀要求，只要求文件是文本格式
```

此外，`tish` 在遇到特殊标记（即 `$$` 和 `~`）时，会将其展开为当前程序的 `pid` 或用户家目录的绝对路径。
```sh
./tish -c "echo ~ && echo \$\$"
```

如果以 `root` 用户身份运行，默认的 `tish::CLI` 对象会将命令提示符替换为没有颜色、且以 `#` 结尾的格式。
