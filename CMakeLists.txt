cmake_minimum_required(VERSION 3.1)

project(knet)

option(BUILD_SHARED_LIBS "build shared libraries" OFF)
option(BUILD_WITHOUT_DEBUG_INFO "trip all debug info" OFF)
option(BUILD_WITH_STATIC_RT "use static runtime" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING "install directory" FORCE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE STRING "archive directory" FORCE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE STRING "library directory" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE STRING "runtime directory" FORCE)

set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "postfix for Debug-built libraries" FORCE)
set(CMAKE_RELWITHDEBINFO_POSTFIX "_rwdi" CACHE STRING "postfix for MinsizeRelease-built libraries" FORCE)
set(CMAKE_MINSIZEREL_POSTFIX "_msr" CACHE STRING "postfix for ReleaseWithDebug-built libraries" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELEASE)
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if (WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif()

if (MSVC)
    add_definitions("/wd4100")
    add_definitions("/W4")

    if (BUILD_WITHOUT_DEBUG_INFO)
        foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "/Z[iI7]" "" ${flag_var} "${${flag_var}}")
        endforeach(flag_var)
    endif()

    if (BUILD_WITH_STATIC_RT)
        foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach(flag_var)
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/
    foreach(flag_var
            CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(APPEND ${flag_var} " -D_FORTIFY_SOURCE=2")
    endforeach(flag_var)
    set(extra_flags "-D_GLIBCXX_ASSERTIONS -fasynchronous-unwind-tables -fexceptions -fstack-protector-strong")
    set(extra_flags "${extra_flags} -Wall -Werror=format-security -Werror=implicit-function-declaration -Wl,-z,defs -Wl,-z,now -Wl,-z,relro")

    set(extra_flags "${extra_flags} -Wl,-unresolved-symbols=ignore-in-shared-libs -Werror -Wno-unused-function -pthread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${extra_flags}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${extra_flags}")

    if (BUILD_WITHOUT_DEBUG_INFO)
        foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "-g" "" ${flag_var} "${${flag_var}}")
        endforeach(flag_var)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -Wl,-s")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -Wl,-s")
    endif()

    if (BUILD_WITH_STATIC_RT)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++")
    endif()
endif()

enable_testing()

#######################################################################################

file(GLOB HDR_LIST
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

file(GLOB SRC_LIST
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

source_group("hdrs" FILES ${HDR_LIST})
source_group("srcs" FILES ${SRC_LIST})
add_library(${CMAKE_PROJECT_NAME} ${HDR_LIST} ${SRC_LIST})

target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/include
    )

if (WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME}
        ws2_32
        )
endif()


install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "include"
    )
install(TARGETS ${CMAKE_PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )

add_subdirectory(test)
add_subdirectory(examples)
