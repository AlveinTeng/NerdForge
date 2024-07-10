cmake_minimum_required (VERSION 2.8.11)
project (Transformation)

find_package(Eigen3 REQUIRED)
include_directories(/opt/homebrew/Cellar/eigen/3.4.0_1/include)
include_directories(/opt/homebrew/Cellar/opencv/4.8.0_6/include)

add_executable (Transformation main.cpp)
