<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

# Development

## Configure and Build the Project Using CMake Presets

The simplest way of configuring and building the project is to use [CMake
Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html). Appropriate
presets for major compilers have been included by default.  You can use `cmake
--list-presets=workflow` to see all available presets.

Here is an example of invoking the `gcc-debug` preset:

```shell
cmake --workflow --preset gcc-debug
```

Generally, there are two kinds of presets, `debug` and `release`.

The `debug` presets are designed to aid development, so they have debuginfo and sanitizers
enabled.

> [!NOTE]
>
> The sanitizers that are enabled vary from compiler to compiler.  See the toolchain files
> under ([`infra/cmake`](infra/cmake/)) to determine the exact configuration used for each
> preset.

The `release` presets are designed for production use, and
consequently have the highest optimization turned on (e.g. `O3`).

## Configure and Build Manually

If the presets are not suitable for your use case, a traditional CMake invocation will
provide more configurability.

To configure, build and test the project manually, you can run this set of commands. Note
that this requires Catch2 to be installed.

```bash
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=20 \
  # Your extra arguments here.
cmake --build build
ctest --test-dir build
```

> [!IMPORTANT]
>
> Beman projects are [passive projects](
> https://github.com/bemanproject/beman/blob/main/docs/beman_standard.md#cmakepassive_projects),
> so you need to specify the C++ version via `CMAKE_CXX_STANDARD` when manually
> configuring the project.

## Dependency Management

### vcpkg

The best way to install the project's dependencies is to use the vcpkg workflow.

To do so, make sure vcpkg is installed and `VCPKG_ROOT` is defined in your environment,
then specify
`-DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"`. Vcpkg will handle
the project's dependencies, including Catch2.

Example commands:

```shell
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
ctest --test-dir build
```

The file `./vcpkg.json` configures the list of dependencies that will be configured by
vcpkg.

### FetchContent

Instead of installing the project's dependencies via a package manager, you can optionally
configure beman.movable_box to fetch them automatically via CMake FetchContent.

To do so, specify
`-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake`. This will
bring in Catch2 automatically along with any other dependency the project may require.

Example commands:

```shell
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake
cmake --build build
ctest --test-dir build
```

The file `./lockfile.json` configures the list of dependencies and versions that will be
acquired by FetchContent.

## Vendor movable-box conformance targets

Besides the reference-implementation suite, the test directory builds the
same suite against each shipping implementation's exposition-only
movable-box, as evidence for the paper's drop-in claim (see
`tests/beman/movable_box/box_under_test.hpp` for the box selection, the
vendor shims, and the catalog of expected divergences):

- `beman.movable_box.tests.libstdcxx_box` — libstdc++'s
  `std::ranges::__detail::__box`, when building against libstdc++ 14 or
  newer (GCC or Clang).
- `beman.movable_box.tests.libcxx_box` — libc++'s
  `std::ranges::__movable_box`, when building against libc++ 17 or newer.
- `beman.movable_box.tests.msvc_stl_box` — the Microsoft STL's
  `std::ranges::_Movable_box`, when building against the MS STL.

Each target is enabled automatically when a probe detects the matching
standard library (with `__cpp_lib_ranges >= 202207L`, i.e. P2494R2
movable-box semantics); no extra configuration is needed.

### The MS STL target on Linux

The MS STL has no native Linux support, but the `msvc_stl_box` target can
be developed on Linux by cross-compiling with `clang-cl` against the MSVC
CRT and Windows SDK, which [xwin](https://github.com/Jake-Shadle/xwin)
downloads without a Windows machine:

```bash
xwin --accept-license --arch x86_64 splat --output ~/xwin

cmake -B build/msvc-stl -S . -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-stl-linux-toolchain.cmake \
  -DXWIN_DIR=$HOME/xwin \
  -DCMAKE_CXX_STANDARD=23 \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake
cmake --build build/msvc-stl
```

Building alone already evaluates every `STATIC_REQUIRE`, `constexpr`
static_assert, and shim-parity check, which is most of the conformance
value. To also run the binaries under `ctest`, install Wine and configure
with `-DCMAKE_CROSSCOMPILING_EMULATOR=wine64` as well. This route is a
local convenience only; the Windows CI jobs are the authority for the MS
STL target.

## Project-specific configure arguments

Project-specific options are prefixed with `BEMAN_MOVABLE_BOX`.
You can see the list of available options with:

```bash
cmake -LH -S . -B build | grep "BEMAN_MOVABLE_BOX" -C 2
```

<details>

<summary>Some project-specific configure arguments</summary>

### `BEMAN_MOVABLE_BOX_BUILD_TESTS`

Enable building tests and test infrastructure. Default: `ON`.
Values: `{ ON, OFF }`.

### `BEMAN_MOVABLE_BOX_BUILD_EXAMPLES`

Enable building examples. Default: `ON`. Values: `{ ON, OFF }`.

### `BEMAN_MOVABLE_BOX_INSTALL_CONFIG_FILE_PACKAGE`

Enable installing the CMake config file package. Default: `ON`.
Values: `{ ON, OFF }`.

This is required so that users of `beman.movable_box` can use
`find_package(beman.movable_box)` to locate the library.

</details>
