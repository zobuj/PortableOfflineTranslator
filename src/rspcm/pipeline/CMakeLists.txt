# Create the executable
add_executable(pipeline main.cpp)

# Ensure Whisper headers are included
target_include_directories(pipeline PRIVATE ${CMAKE_SOURCE_DIR}/whisper)

# Link the executable to the Whisper library and threading support
target_link_libraries(pipeline PRIVATE whisper ${CMAKE_THREAD_LIBS_INIT})

# Ensure the executable is placed in the "bin" folder inside the build directory
set_target_properties(pipeline PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Custom target to build & run the executable
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/bin/pipeline
    DEPENDS pipeline
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
