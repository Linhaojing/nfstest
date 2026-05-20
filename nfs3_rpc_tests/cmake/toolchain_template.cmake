cmake_minimum_required(VERSION 3.14)

# ============================================================================
# Cross-compilation toolchain template for nfs3_rpc_tests
#
# Usage:
#   cmake -B build_cross \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain_arm.cmake \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DBUILD_TESTS=OFF ..
#
# The target sysroot should contain libtirpc, libgtest, and libpthread
# pre-built for the target architecture.
# ============================================================================

# ---- Target system ----------------------------------------------------------
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7-a)

# ---- Toolchain binaries -----------------------------------------------------
# Adjust these paths to match your cross-compiler prefix
set(TARGET_PREFIX   arm-linux-gnueabihf)
set(TOOLCHAIN_ROOT   /opt/gcc-arm/${TARGET_PREFIX})

set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-g++)
set(CMAKE_AR           ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-ar   CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB       ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-ranlib CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP        ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-strip CACHE FILEPATH "Strip")

# ---- Sysroot -----------------------------------------------------------------
# point to a pre-populated sysroot containing target libs and headers
set(SYSROOT_DIR /opt/sysroot/${TARGET_PREFIX})

set(CMAKE_SYSROOT                    ${SYSROOT_DIR})
set(CMAKE_FIND_ROOT_PATH             ${SYSROOT_DIR})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---- pkg-config -------------------------------------------------------------
# Give pkg-config a chance to find host .pc files inside the sysroot
set(PKG_CONFIG_EXECUTABLE   ${TOOLCHAIN_ROOT}/bin/${TARGET_PREFIX}-pkg-config
    CACHE FILEPATH "pkg-config for the target")
set(ENV{PKG_CONFIG_PATH}    "${SYSROOT_DIR}/usr/lib/pkgconfig:${SYSROOT_DIR}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${SYSROOT_DIR}")

# ---- C / C++ flags (optional) -----------------------------------------------
# Tune to your target CPU. Remove or adjust if the generic flags work.
set(ARCH_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb")
set(CMAKE_C_FLAGS_INIT   "${ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ARCH_FLAGS}")

# ---- Skipping tests on cross-compiled targets (optional) --------------------
# Tests usually cannot run on the build host; disable unless you plan to
# deploy them to the target and run via ssh / remote runner.
set(BUILD_TESTS    OFF CACHE BOOL "Disable tests for cross build")
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples for cross build")
