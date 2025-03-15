cmake_minimum_required(VERSION 3.10)
project(FindTimeStamp)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Add executable
add_executable(hist_thresh main.cpp)

# Link OpenCV libraries
target_link_libraries(hist_thresh ${OpenCV_LIBS})