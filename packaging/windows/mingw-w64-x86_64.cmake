# CMake toolchain to cross-compile the emulator for 64-bit Windows with MinGW-w64 (used by make-zip.sh)
#
# TK2000_SDL2_DIR: the x86_64-w64-mingw32 directory of the SDL2 MinGW development package

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Look for headers and libraries only in the MinGW sysroot and the SDL2 package, never in the Linux system
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 "${TK2000_SDL2_DIR}")
set(CMAKE_PREFIX_PATH "${TK2000_SDL2_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# The C and C++ runtimes go inside the executable, so only SDL2.dll has to be shipped next to it
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
