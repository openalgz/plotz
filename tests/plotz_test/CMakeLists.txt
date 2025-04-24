project(plotz_test)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE test_common)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})