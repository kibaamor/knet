add_executable(async_client async_client.cpp echo_conn.h echo_conn.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.cpp)
target_link_libraries(async_client ${CMAKE_PROJECT_NAME})
set_property(TARGET async_client PROPERTY FOLDER "examples/client")

add_executable(sync_client sync_client.cpp echo_conn.h echo_conn.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../echo_mgr.cpp)
target_link_libraries(sync_client ${CMAKE_PROJECT_NAME})
set_property(TARGET sync_client PROPERTY FOLDER "examples/client")