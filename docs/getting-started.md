# Getting Started

This guide will walk you through how to use the Nezumi Engine:

1. **Requirements**: The things your system needs to build the engine.
2. **Build**: How to build the engine tests and examples.
3. **Test**: Running the engine tests.
4. **Examples**: Running the engine examples.
5. **Build a Game**: Building your own game using the engine.
6. **Profile** Using the engine's profiling tool to record performance.

## 1. Requirements

Before we get started, ensure your system has these requirements.

### 1a. Compiler

Building the engine requires a C/C++ compiler with C++20 support.

| Compiler                                       | Version | Purpose          |
|------------------------------------------------|---------|------------------|
| [GCC/G++](https://gcc.gnu.org/)                | 10+     | Linux compiler   |
| [MSVC](https://visualstudio.microsoft.com/vs/) | 2019+   | Windows compiler |
| [Clang](https://clang.llvm.org/)               | 12+     | macOS compiler   |

### 1b. Tools

Building the engine requires these tools.

| Tool                              | Version | Purpose                        |
|-----------------------------------|---------|--------------------------------|
| [Git](https://git-scm.com/)       |         | Download dependencies          |
| [CMake](https://cmake.org/)       | 3.25+   | Build dependencies and project |
| [OpenGL](https://www.opengl.org/) | 4.6+    | Rendering API                  |

### 1c. Linux system packages

SDL3 is built from source and needs the development headers for your display server and audio stack. On Debian/Ubuntu:

```sh
sudo apt install build-essential git cmake \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \
    libxtst-dev libxkbcommon-dev libwayland-dev libegl-dev libgl-dev \
    libasound2-dev libpulse-dev
```

See the [SDL3 Linux build dependencies](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies) for other distributions. Windows and macOS need nothing extra.

### 1d. Dependencies

Dependencies are downloaded and built automatically by CMake the first time you configure. No manual installation is required.

| Library                                                  | Version | Purpose                          |
|----------------------------------------------------------|---------|----------------------------------|
| [SDL3](https://libsdl.org/)                              | 3.4.2   | Windowing, input, OpenGL context |
| [GLAD](https://github.com/Dav1dde/glad)                  | 2.0.8   | OpenGL 4.6 Core loader (vendored in`thirdparty/glad`) |
| [GLM](https://github.com/g-truc/glm)                     | 1.0.1   | Mathematics library              |
| [FreeType](https://freetype.org/)                        | 2.13.3  | Font rendering                   |
| [STB](https://github.com/nothings/stb)                   | pinned  | Image loading (header-only)      |
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | 1.1.1   | Noise generation (header-only)   |

## 2. Build 

```sh
cmake --preset debug
cmake --build --preset debug
```

Use `release` in place of `debug` for an optimized build. Output goes to `.build/<preset>/`. The first configure downloads the dependencies, so it takes a few minutes; later builds reuse them.

On Linux and macOS, `tools/build.sh` wraps these commands and can also clean and run tests (`tools/build.sh --help` lists the options):

```sh
tools/build.sh --preset release --test
```

IDEs that understand CMake presets (Visual Studio, VS Code, CLion) can open the folder directly and pick the preset from a menu.

## 3. Test

To run the engine tests:

```sh
ctest --preset debug              # all tests
ctest --preset debug -L headless  # skip tests that need a display
```

## 4. Examples

To run an engine example:

```sh
.build/debug/examples/bin/00-hello-world
```

## 5. Build a Game

TODO: link to nezumi-hello-world git repo example.

Add this to your game's `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(nezumi
    GIT_REPOSITORY https://github.com/<you>/nezumi.git
    GIT_TAG        v0.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nezumi)

add_executable(my_game src/main.cpp)
target_link_libraries(my_game PRIVATE Nezumi::Engine)

# Copy the engine's runtime assets and your own assets/ folder next to the
# executable after every build
nezumi_copy_assets(my_game ENGINE)
```

Only the engine is built when Nezumi is used this way; its examples and tests
are skipped.

## 6. Profile

On Linux, `tools/profile.sh` records an example with `perf` and renders a flame graph SVG into `.profile/`. Profile the `profile` preset, an optimized build that keeps the debug symbols and frame pointers perf needs to unwind stacks:

```sh
tools/build.sh --preset profile
tools/profile.sh 01-renderer
```

Requires `perf` and `perl`. The FlameGraph scripts are cloned into `.profile/`
on first use.
