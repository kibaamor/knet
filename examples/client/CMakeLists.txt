set(COMMON_FILES
    cecho_conn.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_conn.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.cpp)

add_executable(async_client async_client.cpp ${COMMON_FILES})
target_link_libraries(async_client ${PROJECT_NAME} ${PROJECT_NAME}_compiler_flags)
set_property(TARGET async_client PROPERTY FOLDER "examples/client")

add_executable(sync_client sync_client.cpp ${COMMON_FILES})
target_link_libraries(sync_client ${PROJECT_NAME} ${PROJECT_NAME}_compiler_flags)
set_property(TARGET sync_client PROPERTY FOLDER "examples/client")
