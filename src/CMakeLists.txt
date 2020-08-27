configure_file(
    ${PROJECT_SOURCE_DIR}/include/knet/knetcfg.h.in
    ${PROJECT_SOURCE_DIR}/include/knet/knetcfg.h
)

if (CMAKE_BUILD_TYPE STREQUAL DEBUG OR CMAKE_BUILD_TYPE STREQUAL RELWITHDEBINFO)
    add_definitions(-DDEBUG)
    message("knet enable debug")
endif()

if (WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)

    set(knet_platform_postfix "win")
    set(knet_poller_postfix "iocp")
else ()
    set(knet_platform_postfix "unix")

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(knet_poller_postfix "epoll")
    else ()
        set(knet_poller_postfix "kqueue")
    endif ()
endif()
message("knet_platform_postfix: ${knet_platform_postfix}")
message("knet_poller_postfix: ${knet_poller_postfix}")

#######################################################################################

file(GLOB HDR_LIST
    "${PROJECT_SOURCE_DIR}/include/knet/*.h"
)
source_group("hdrs" FILES ${HDR_LIST})

file(GLOB SRC_LIST
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/internal/*.h"
    "${PROJECT_SOURCE_DIR}/src/internal/*.cpp"
)
source_group("srcs" FILES ${SRC_LIST})

file(GLOB_RECURSE EXTRA_SRC_LIST
    "${PROJECT_SOURCE_DIR}/src/*${knet_platform_postfix}.h"
    "${PROJECT_SOURCE_DIR}/src/*${knet_platform_postfix}.cpp"
    "${PROJECT_SOURCE_DIR}/src/*${knet_poller_postfix}.h"
    "${PROJECT_SOURCE_DIR}/src/*${knet_poller_postfix}.cpp"
)
source_group("srcs\\extra" FILES ${EXTRA_SRC_LIST})

add_library(${PROJECT_NAME} ${HDR_LIST} ${SRC_LIST} ${EXTRA_SRC_LIST})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        "EXPORTING_KNET"
        $<$<CONFIG:Debug>:KNET_DEBUG>
        $<$<CONFIG:RelWithDebInfo>:KNET_DEBUG>
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include>
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ws2_32 knet_compiler_flags)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Threads::Threads ${CMAKE_DL_LIBS} knet_compiler_flags)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    )
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_compiler_flags
    DESTINATION lib
    EXPORT ${PROJECT_NAME}Targets
    )

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
# generate the config file that is includes the exports
configure_package_config_file(${PROJECT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
    COMPATIBILITY AnyNewerVersion
)

# install the configuration file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
