# beman.movable_box: A non-exposition-only movable-box

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable line-length -->
[![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#the-beman-library-maturity-model)
[![Continuous Integration Tests](https://github.com/bemanproject/movable_box/actions/workflows/ci_tests.yml/badge.svg)](https://github.com/bemanproject/movable_box/actions/workflows/ci_tests.yml)
[![Lint Check (pre-commit)](https://github.com/bemanproject/movable_box/actions/workflows/pre-commit-check.yml/badge.svg)](https://github.com/bemanproject/movable_box/actions/workflows/pre-commit-check.yml)
[![Coverage](https://coveralls.io/repos/github/bemanproject/movable_box/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/movable_box?branch=main)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)
<!-- markdownlint-restore -->

`beman.movable_box` provides `movable_box<T>`, the ranges *movable-box* wrapper: a
`move_constructible` object wrapper that synthesizes copy/move assignment (via
destroy-then-construct when `T` itself is not assignable) so that the result always models
`movable`, as a public, non-exposition-only library type.

**Implements**: `std::movable_box` proposed in the draft paper
([papers/movable_box.md](papers/movable_box.md)); paper number not yet assigned.

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.movable_box` is licensed under the Apache License v2.0 with LLVM Exceptions.

## Usage

`movable_box<T>` stores a `move_constructible` object and gives it assignability, even when `T`
itself is not assignable -- exactly the tool range adaptors use to hold predicates, projections,
and transformation functions inside otherwise-`movable` view types.

```cpp
#include <beman/movable_box/movable_box.hpp>

#include <utility>

constexpr auto make_adder(int captured) {
    return [captured](int x) { return x + captured; };
}

using Adder = decltype(make_adder(0));

int main() {
    // A capturing lambda's closure type has a deleted copy/move assignment operator, so it
    // cannot be stored directly in a type that must remain assignable.
    beman::movable_box::movable_box<Adder> box(make_adder(3));

    // Assignability is synthesized: even though `Adder` has no operator=, `box` can be
    // reassigned. Internally this destroys and reconstructs the contained callable.
    box = beman::movable_box::movable_box<Adder>(make_adder(-3));

    return (*box)(10); // 7
}
```

Full runnable examples can be found in [`examples/`](examples/).

## Dependencies

### Build Environment

This project requires at least the following to build:

* A C++ compiler that conforms to the C++20 standard or greater
* CMake 3.30 or later
* (Test Only) Catch2

You can disable building tests by setting CMake option `BEMAN_MOVABLE_BOX_BUILD_TESTS` to
`OFF` when configuring the project.

### Supported Platforms

| Compiler   | Version | C++ Standards | Standard Library  |
|------------|---------|---------------|-------------------|
| GCC        | 16-13   | C++26-C++20   | libstdc++         |
| GCC        | 12-11   | C++23-C++20   | libstdc++         |
| Clang      | 22-19   | C++26-C++20   | libstdc++, libc++ |
| Clang      | 18      | C++26-C++20   | libc++            |
| Clang      | 18      | C++23-C++20   | libstdc++         |
| Clang      | 17      | C++26-C++20   | libc++            |
| Clang      | 17      | C++20         | libstdc++         |
| AppleClang | latest  | C++26-C++20   | libc++            |
| MSVC       | latest  | C++23         | MSVC STL          |

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).

## Integrate beman.movable_box into your project

### Build

You can build movable_box using a CMake workflow preset:

```bash
cmake --workflow --preset gcc-release
```

To list available workflow presets, you can invoke:

```bash
cmake --list-presets=workflow
```

For details on building beman.movable_box without using a CMake preset, refer to the
[Contributing Guidelines](CONTRIBUTING.md).

### Installation

#### Vcpkg

The preferred way to install movable_box is via vcpkg. To do so, after installing vcpkg
itself, you need to add support for the Beman project's [vcpkg
registry](https://github.com/bemanproject/vcpkg-registry) by configuring a
`vcpkg-configuration.json` file (which movable_box [provides](vcpkg-configuration.json)).

Then, simply run `vcpkg install beman-movable-box`.

#### Manual

To install beman.movable_box globally after building with the `gcc-release` preset, you can
run:

```bash
sudo cmake --install build/gcc-release
```

Alternatively, to install to a prefix, for example `/opt/beman`, you can run:

```bash
sudo cmake --install build/gcc-release --prefix /opt/beman
```

This will generate the following directory structure:

```txt
/opt/beman
├── include
│   └── beman
│       └── movable_box
│           ├── movable_box.hpp
│           └── ...
└── lib
    └── cmake
        └── beman.movable_box
            ├── beman.movable_box-config-version.cmake
            ├── beman.movable_box-config.cmake
            └── beman.movable_box-targets.cmake
```

### CMake Configuration

If you installed beman.movable_box to a prefix, you can specify that prefix to your CMake
project using `CMAKE_PREFIX_PATH`; for example, `-DCMAKE_PREFIX_PATH=/opt/beman`.

You need to bring in the `beman.movable_box` package to define the `beman::movable_box` CMake
target:

```cmake
find_package(beman.movable_box REQUIRED)
```

You will then need to add `beman::movable_box` to the link libraries of any libraries or
executables that include `beman.movable_box` headers.

```cmake
target_link_libraries(yourlib PUBLIC beman::movable_box)
```

### Using beman.movable_box

To use `beman.movable_box` in your C++ project,
include an appropriate `beman.movable_box` header from your source code.

```c++
#include <beman/movable_box/movable_box.hpp>
```
