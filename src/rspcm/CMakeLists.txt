cmake_minimum_required(VERSION 3.10)
project(TranslationPipeline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable threading support
find_package(Threads REQUIRED)

# Add the Whisper library
add_subdirectory(whisper.cpp)
add_subdirectory(llama.cpp)

# Add the new folder containing the main executable
add_subdirectory(pipeline)
