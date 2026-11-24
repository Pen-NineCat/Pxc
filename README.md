# Pxc Language Project

Welcome to the Pxc Language Project! 

This project is dedicated to the development and documentation of the Pxc programming language.

## Language design philosophy

Don't force programmers to design the entire program's object relationships just to manage memory; but also don't deprive programmers of control over memory and resources for the sake of convenience.

## Project Directory

```
Code
| - PxcCommon  // Basic Environment
| - PLang   // Pxc Complier
| - PLangLs // Pxc Language Service
| - PLib    // Pxc Standard Library
| - PPak    // Pxc Package Manager
| - Tests
| - Utils
PxcReference    // Pxc Language reference and renderer
```

## Third party dependencies

Pxc uses vcpkg to manage third-party dependencies, see `vcpkg.json` for the full list and `vcpkg-configuration.json` for the pinned vcpkg baseline. LLVM is the heavy one: the first configure builds (or restores from the vcpkg binary cache) a full LLVM, so expect that step to take a while. `magic-enum` is used by the compiler itself and `gtest` by the unit tests; both are small and are installed together with everything else.

## Quick start

### Prepare

Clone repo

```
git clone https://github.com/Pen-NineCat/Pxc.git
```

You need CMake 4.0 or newer, Ninja, and a compiler (MSVC, GCC or Clang).

Ensure that you have installed vcpkg and vcpkg can contact github and install ports.

Set the `VCPKG_ROOT` environment variable, which should point to the path where `vcpkg.exe` is located, for example:
```
set VCPKG_ROOT=E:\vcpkg
```

Alternatively, you can also use `CMakeUserPresets.json` to override the project `CMakePresets.json` configuration; `PXC_CMAKE_VCPKG_TOOLCHAIN_FILE` points at another vcpkg checkout.

For more information about vcpkg, see the [vcpkg github](https://github.com/microsoft/vcpkg).

### Build

The presets only pass the architecture by name (`PXC_CMAKE_ARCHITECTURE`, `x64` or `x86`); the compiler keeps its own default target. Configure fails when the two disagree, instead of silently building another architecture.

> An `out/` directory created by an older checkout has to be deleted once: the vcpkg install tree is now one per host system and architecture, and the old `CMakeCache.txt` keeps pointing at the previous toolchain file.

#### MSVC (Windows)

`cl.exe` comes from the vcvarsall environment, so enter it first. To x64 target:

```bat
pushd "path\to\your\Visual Studio\VC\Auxiliary\Build"
vcvarsall.bat x64
popd
cmake --preset x64-debug
cmake --build --preset x64-debug
```

To x86 target, use `vcvarsall.bat x86` and the `x86-debug` preset instead. Inside a Developer Command Prompt, `%VSINSTALLDIR%\VC\Auxiliary\Build` is the same directory.

#### GCC / Clang (Linux)

```bash
cmake --preset x64-debug
cmake --build --preset x64-debug
```

The x86 presets add `-m32`, so they need the 32-bit development packages of your distribution (`gcc-multilib`, `g++-multilib` and `libc6-dev-i386` on Ubuntu); vcpkg builds the dependencies for the same target as well.

### Tests

Tests are off by default, `PXC_CMAKE_ENABLE_TESTS` turns them on. The test dependencies are installed with the rest of the manifest, so the vcpkg tree does not change when the tests are toggled:

```bash
cmake --preset x64-debug-tests
cmake --build --preset x64-debug-tests
ctest --preset x64-debug-tests
```

Every architecture has a test preset (`x64-debug-tests`, `x86-debug-tests`, `x64-release-tests`, `x86-release-tests`), configure, build and test them with the matching preset name.

### AddressSanitizer

`PXC_CMAKE_ENABLE_ASAN` builds the Debug configuration with `/fsanitize=address`. It is implemented for MSVC only; other compilers warn and ignore it.

```bat
cmake --preset x64-debug -DPXC_CMAKE_ENABLE_ASAN=ON
cmake --build --preset x64-debug
```

The ASAN runtime (`clang_rt.asan_dynamic-x86_64.dll`) stays in the MSVC toolset directory, so run the produced binaries from a Developer Command Prompt, or the process exits with `0xC0000135` (DLL not found).

## Windows Segment heap

If you build with Visual Studio and the MSVC compiler, you can enable Windows Segment heap support by setting the `PXC_CMAKE_ENABLE_SEGMENT_HEAP` CMake option to `ON`.

The build scripts look for `<VSINSTALLDIR>/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/Microsoft/SegmentHeap.cmake`, so `VSINSTALLDIR` has to point at a Visual Studio installation directory that contains `Common7`, for example:
```
set VSINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community
```

Configure names the exact path it looked for when the file is missing.

For more information about Segment Heap, see the [Visual Studio C++ blog post](https://devblogs.microsoft.com/cppblog/segment-heap-support-for-c-projects-in-visual-studio/).

## Continuous integration

`.github/workflows/ci-format.yml` and `.github/workflows/ci-build-test.yml` run on pull requests, but only for PRs labelled `ci:cpp`. The label is the opt-in for the C++ CI. The build matrix is Windows and Linux, x64 and x86, in Release; the formatting check uses a pinned clang-format.

## Docs

The Pxc language documentation is rendered with mdBook. The rendering setup and the third-party notices live in `PxcReference`; the source content is under `PxcReference/src`. The site is built by the `.github/workflows/docs-pages.yml` workflow (manual trigger) and published to [GitHub Pages](https://pen-ninecat.github.io/Pxc).
