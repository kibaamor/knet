add_executable(async_server async_server.cpp socket_listener.h socket_listener.cpp)
target_link_libraries(async_server ${CMAKE_PROJECT_NAME})
set_property(TARGET async_server PROPERTY FOLDER "examples")

add_executable(sync_server sync_server.cpp socket_listener.h  socket_listener.cpp)
target_link_libraries(sync_server ${CMAKE_PROJECT_NAME})
set_property(TARGET sync_server PROPERTY FOLDER "examples")