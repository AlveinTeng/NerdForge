cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
include_directories(/opt/homebrew/Cellar/eigen/3.4.0_1/include)
include_directories(/opt/homebrew/Cellar/opencv/4.8.0_6/include)
set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp Triangle.hpp Triangle.cpp)
target_link_libraries(Rasterizer ${OpenCV_LIBRARIES})
