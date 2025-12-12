# CMake Toolchain file for crosscompiling on x86_64(aks aarch64) macos.

# Target operating system name
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Names of compilers
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_OSX_ARCHITECTURES x86_64)
set(CMAKE_OSX_ARCHITECTURES_OPTION -DCMAKE_OSX_ARCHITECTURES=x86_64)
