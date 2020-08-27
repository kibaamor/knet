cmake_minimum_required(VERSION 3.9)

project(knet VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "postfix for Debug-built libraries" FORCE)
set(CMAKE_MINSIZEREL_POSTFIX "_msr" CACHE STRING "postfix for MinsizeRelease-built libraries" FORCE)
set(CMAKE_RELWITHDEBINFO_POSTFIX "_rwdi" CACHE STRING "postfix for ReleaseWithDebug-built libraries" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(KNET_BUILD_EXAMPLE "Build exmaple" OFF)
option(KNET_BUILD_TEST "Build test" OFF)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_11)
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(${PROJECT_NAME}_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

add_subdirectory(src)

if (KNET_BUILD_EXAMPLE)
    message("knet build examples")
    add_subdirectory(examples)
endif()

if (KNET_BUILD_TEST)
    include(CTest)
    message("knet build test")
    add_subdirectory(test)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
include(CPack)
