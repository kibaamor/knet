set(COMMON_FILES
    secho_conn.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_conn.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.cpp)

add_executable(async_server async_server.cpp ${COMMON_FILES})
target_link_libraries(async_server ${PROJECT_NAME} ${PROJECT_NAME}_compiler_flags)
set_property(TARGET async_server PROPERTY FOLDER "examples/server")

add_executable(sync_server sync_server.cpp ${COMMON_FILES})
target_link_libraries(sync_server ${PROJECT_NAME} ${PROJECT_NAME}_compiler_flags)
set_property(TARGET sync_server PROPERTY FOLDER "examples/server")
