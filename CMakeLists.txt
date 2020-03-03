cmake_minimum_required(VERSION 3.1)

project(knet)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "postfix for Debug-built libraries" FORCE)
set(CMAKE_RELWITHDEBINFO_POSTFIX "_rwdi" CACHE STRING "postfix for MinsizeRelease-built libraries" FORCE)
set(CMAKE_MINSIZEREL_POSTFIX "_msr" CACHE STRING "postfix for ReleaseWithDebug-built libraries" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELEASE)
endif()
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if (WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif()

if (MSVC)
    add_definitions("/wd4100")
    add_definitions("/W4")
endif()

enable_testing()

#######################################################################################

file(GLOB HDR_LIST
    "${PROJECT_SOURCE_DIR}/include/*.h"
)

file(GLOB SRC_LIST
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

source_group("hdrs" FILES ${HDR_LIST})
source_group("srcs" FILES ${SRC_LIST})
add_library(${PROJECT_NAME} ${HDR_LIST} ${SRC_LIST})

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    )

if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        ws2_32
        )
endif()


install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION "include"
    )
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )

add_subdirectory(test)
add_subdirectory(examples)
