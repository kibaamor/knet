cmake_minimum_required(VERSION 3.15)

project(knet VERSION 1.0.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "postfix for Debug-built libraries" FORCE)
set(CMAKE_MINSIZEREL_POSTFIX "_msr" CACHE STRING "postfix for MinsizeRelease-built libraries" FORCE)
set(CMAKE_RELWITHDEBINFO_POSTFIX "_rwdi" CACHE STRING "postfix for ReleaseWithDebug-built libraries" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

option(KNET_ENABLE_LOG "Show log on cerr" ON)
option(KNET_BUILD_EXAMPLE "Build exmaple" ON)
option(KNET_BUILD_TEST "Build test" ON)

message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message("KNET_ENABLE_LOG: ${KNET_ENABLE_LOG}")
message("KNET_BUILD_EXAMPLE: ${KNET_BUILD_EXAMPLE}")
message("KNET_BUILD_TEST: ${KNET_BUILD_TEST}")

#######################################################################################
# common compiler flags

set(gcc_like_cxx "$<CXX_COMPILER_ID:ARMClang,AppleClang,Clang,GNU>")
set(msvc_cxx "$<CXX_COMPILER_ID:MSVC>")
set(debug_like_build "$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>")
set(gcc_debug_like "$<AND:${gcc_like_cxx},${debug_like_build}>")

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_11)
target_compile_options(${PROJECT_NAME}_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-D_GNU_SOURCE;-Wall;-Wextra;-Wpedantic;-Wno-unused-function;-Wno-unused-parameter;-Wno-format-nonliteral;-Wno-gnu-statement-expression;-Wshadow;-Wformat=2>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W4;-wd4100>>"
    # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/
    "$<${gcc_debug_like}:$<BUILD_INTERFACE:-D_FORTIFY_SOURCE=2;-D_GLIBCXX_ASSERTIONS;-fasynchronous-unwind-tables;-fstack-protector-all>>"
    "$<$<PLATFORM_ID:Windows>:$<BUILD_INTERFACE:-DUNICODE;-D_UNICODE>>"
)

#######################################################################################
# subprojects

add_subdirectory(src)

if (KNET_BUILD_EXAMPLE)
    add_subdirectory(examples)
endif()

if (KNET_BUILD_TEST)
    include(CTest)
    add_subdirectory(test)
endif()

#######################################################################################
# installation

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
include(CPack)
