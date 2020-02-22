add_executable(async_client async_client.cpp socket_listener.h socket_listener.cpp)
target_link_libraries(async_client ${CMAKE_PROJECT_NAME})
set_property(TARGET async_client PROPERTY FOLDER "examples")

add_executable(sync_client sync_client.cpp socket_listener.h socket_listener.cpp)
target_link_libraries(sync_client ${CMAKE_PROJECT_NAME})
set_property(TARGET sync_client PROPERTY FOLDER "examples")