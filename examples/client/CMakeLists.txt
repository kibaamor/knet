add_executable(async_client async_client.cpp echo_conn.h echo_conn.cpp)
target_link_libraries(async_client ${CMAKE_PROJECT_NAME})
set_property(TARGET async_client PROPERTY FOLDER "examples")

add_executable(sync_client sync_client.cpp echo_conn.h echo_conn.cpp)
target_link_libraries(sync_client ${CMAKE_PROJECT_NAME})
set_property(TARGET sync_client PROPERTY FOLDER "examples")