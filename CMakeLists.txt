cmake_minimum_required(VERSION 2.8)

project( AudioVisualizer )

find_package( OpenCV REQUIRED )

set(CMAKE_CXX_CPPLINT "cpplint")

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)

add_executable( main main.cpp )


target_link_libraries( main PRIVATE ${OpenCV_LIBS} src )

target_include_directories(main PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
    )