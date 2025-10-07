cmake_minimum_required(VERSION 3.22)
project(flaming)

set(CMAKE_CXX_STANDARD 23)

find_package(glm)
find_package(glfw3)

add_subdirectory(lib/volk)

add_executable(flaming main.cpp src/desu.h src/desu.cpp)

target_link_libraries(flaming glfw volk_headers shaderc_shared portaudio fftw3 fftw3f fftw3_threads fftw3f_threads m pthread)
