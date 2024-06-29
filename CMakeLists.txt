cmake_minimum_required(VERSION 3.27)
project(VulkanProject)

set(CMAKE_CXX_STANDARD 23)

file(COPY "source/shaders/" DESTINATION "bin/shaders/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(VulkanProject main.cpp
  source/VulkanApp.cpp
  source/VulkanApp.h
  source/Window.cpp
  source/Window.h)

target_include_directories(${PROJECT_NAME} PUBLIC
  libraries/glfw-3.4.bin.WIN64/include
  libraries/glm
  C:/VulkanSDK/1.3.283.0/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
  libraries/glfw-3.4.bin.WIN64/lib-mingw-w64
)

set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.283.0")
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
  glfw3
  ${Vulkan_LIBRARIES}
)