add_executable(userdata userdata.cpp doctest.h)
target_link_libraries(userdata ${CMAKE_PROJECT_NAME})
set_property(TARGET userdata PROPERTY FOLDER "test")
add_test(NAME test_userdata COMMAND $<TARGET_FILE:userdata>)