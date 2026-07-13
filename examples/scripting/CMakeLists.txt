add_executable(scripting-example main.cpp)
target_link_libraries(scripting-example PRIVATE typengine::typengine)

add_custom_command(TARGET scripting-example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:scripting-example>/scripts
)
