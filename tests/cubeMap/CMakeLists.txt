project("cubeMap")

add_executable(${PROJECT_NAME} main.cpp
  ../common/gui.h)

target_link_libraries(${PROJECT_NAME} PRIVATE VulkanEngine)