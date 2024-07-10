cmake_minimum_required(VERSION 3.27)
project(VulkanProject)

set(CMAKE_CXX_STANDARD 23)

file(COPY "assets/" DESTINATION "bin/assets/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(VulkanProject main.cpp
  source/VulkanApp.cpp
  source/VulkanApp.h
  source/Window.cpp
  source/Window.h
  source/DebugMessenger.cpp
  source/DebugMessenger.h
  source/objects/RenderObject.cpp
  source/objects/RenderObject.h
  source/Buffers.cpp
  source/Buffers.h
  source/objects/Model.cpp
  source/objects/Model.h
  source/objects/Texture.cpp
  source/objects/Texture.h)

target_include_directories(${PROJECT_NAME} PUBLIC
  libraries/glfw-3.4.bin.WIN64/include
  libraries/glm
  C:/VulkanSDK/1.3.283.0/include
  libraries/stb
  libraries/assimp-5.4.2/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
  libraries/glfw-3.4.bin.WIN64/lib-mingw-w64
  libraries/assimp-5.4.2/lib
)

file(COPY "libraries/assimp-5.4.2/lib/libassimp-5.dll" DESTINATION "bin/")

set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.283.0")
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
  glfw3
  ${Vulkan_LIBRARIES}
  assimp
)
