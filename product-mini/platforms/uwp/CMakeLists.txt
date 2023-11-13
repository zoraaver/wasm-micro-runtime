# Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.25)
cmake_policy(VERSION 3.22)

project (iwasm C ASM CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.17763.0")

set (WAMR_BUILD_PLATFORM "windows")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_compile_definitions(
    COMPILING_WASM_RUNTIME_API=1
    _CRT_SECURE_NO_WARNINGS=1
)

# https://learn.microsoft.com/en-us/cpp/build/reference/sdl-enable-additional-security-checks
add_compile_options(/sdl-)

set (DEBUG_LINK_OPTIONS /SUBSYSTEM:CONSOLE /OPT:NOREF /OPT:NOICF)
set (RELEASE_LINK_OPTIONS /SUBSYSTEM:CONSOLE /INCREMENTAL:NO)
add_link_options("$<IF:$<CONFIG:Debug>,${DEBUG_LINK_OPTIONS},${RELEASE_LINK_OPTIONS}>")

# Set WAMR_BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "AARCH64[sub]", "ARM[sub]", "THUMB[sub]", "MIPS", "XTENSA"
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  else ()
    message(FATAL_ERROR "Unsupported build target platform!")
  endif ()
endif ()

if (${WAMR_BUILD_TARGET} STREQUAL "X86_64" OR ${WAMR_BUILD_TARGET} STREQUAL "AMD_64")
  set (WINDOWS_APP_ARCH x64)
elseif (${WAMR_BUILD_TARGET} STREQUAL "X86_32")
  set (WINDOWS_APP_ARCH x86)
elseif (${WAMR_BUILD_TARGET} MATCHES "ARM.*")
  set (WINDOWS_APP_ARCH ARM)
elseif (${WAMR_BUILD_TARGET} MATCHES "AARCH64.*")
  set (WINDOWS_APP_ARCH ARM64)
else()
  message(FATAL_ERROR "Unsupported architecture for UWP app - ${WAMR_BUILD_TARGET}")
endif()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Enable AOT by default.
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_JIT)
  # Disable JIT by default.
  set (WAMR_BUILD_JIT 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Enable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 1)
endif ()

if (NOT DEFINED WAMR_BUILD_FAST_INTERP)
  # Enable fast interpreter
  set (WAMR_BUILD_FAST_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_MULTI_MODULE)
  # Enable multiple modules
  set (WAMR_BUILD_MULTI_MODULE 0)
endif ()

if (NOT DEFINED WAMR_BUILD_LIB_PTHREAD)
  # Disable pthread library by default
  set (WAMR_BUILD_LIB_PTHREAD 0)
endif ()

if (NOT DEFINED WAMR_BUILD_MINI_LOADER)
  # Disable wasm mini loader by default
  set (WAMR_BUILD_MINI_LOADER 0)
endif ()

if (NOT DEFINED WAMR_BUILD_SIMD)
  # Enable SIMD by default
  set (WAMR_BUILD_SIMD 1)
endif ()

if (NOT DEFINED WAMR_BUILD_DEBUG_INTERP)
  # Disable Debug feature by default
  set (WAMR_BUILD_DEBUG_INTERP 0)
endif ()

if (WAMR_BUILD_DEBUG_INTERP EQUAL 1)
  set (WAMR_BUILD_FAST_INTERP 0)
  set (WAMR_BUILD_MINI_LOADER 0)
  set (WAMR_BUILD_SIMD 0)
endif ()

if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  set (CMAKE_C_STANDARD 11)
  if (MSVC)
    add_compile_options(/experimental:c11atomics)
  endif()
else()
  set (CMAKE_C_STANDARD 99)
endif()

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT iwasm)

function(deploy_content TARGET LOCATION)
    set(SOURCES ${ARGN})
    target_sources(${TARGET} PRIVATE ${SOURCES})
    set_source_files_properties(${SOURCES} PROPERTIES
        VS_DEPLOYMENT_CONTENT 1
        VS_DEPLOYMENT_LOCATION ${LOCATION}
    )
    source_group(Deployed/${LOCATION} FILES ${SOURCES})
endfunction()

function(create_uwp_app TARGET TARGET_PACKAGE)
    set(TARGET_SOURCES "${ARGN}")

    configure_file(Package.appxmanifest.in Package.appxmanifest @ONLY)

    source_group(TREE ${WAMR_ROOT_DIR} PREFIX Sources FILES ${TARGET_SOURCES})

    set(CONFIG_FILES
        Package.appxmanifest.in
        ${CMAKE_CURRENT_BINARY_DIR}/Package.appxmanifest
    )

    source_group(Config FILES ${CONFIG_FILES})

    add_executable(${TARGET} WIN32
        ${TARGET_SOURCES}
        ${CONFIG_FILES}
    )

    deploy_content(${TARGET} Assets
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/SplashScreen.scale-200.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Square44x44Logo.scale-200.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Square44x44Logo.targetsize-24_altform-unplated.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Square150x150Logo.scale-200.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/StoreLogo.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Wide310x150Logo.scale-200.png
    )

    deploy_content(${TARGET} .
        ${WAMR_ROOT_DIR}/tests/wamr-test-suites/workspace/wasi-testsuite/tests/c/testsuite/fs-tests.dir
    )

    set_target_properties(${TARGET} PROPERTIES
        FOLDER Applications
        VS_GLOBAL_AppxBundlePlatforms "${WINDOWS_APP_ARCH}"
        VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "${WINDOWS_TARGET_PLATFORM_MIN_VERSION}"
    )

    add_custom_target(${TARGET_PACKAGE}
        COMMAND msbuild ${CMAKE_BINARY_DIR}/${TARGET}.vcxproj
                /t:Build
                /p:Configuration=$<CONFIG>
                /p:Platform="${WINDOWS_APP_ARCH}"
                /p:AppxBundle="Always"
                /p:AppxBundlePlatforms="${WINDOWS_APP_ARCH}"
                /p:AppxPackageIsForStore="true"
                /p:BuildAppxUploadPackageForUap="true"
                /p:UapAppxPackageBuildMode="StoreUpload"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Packaging ${TARGET} as ${TARGET_PACKAGE}..."
    )

    set_target_properties(${TARGET_PACKAGE} PROPERTIES FOLDER Packages)
endfunction()

create_uwp_app(iwasm package-iwasm ${WAMR_RUNTIME_LIB_SOURCE} ${UNCOMMON_SHARED_SOURCE} main.cpp)