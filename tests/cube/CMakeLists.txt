project("cube")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE VulkanEngine)

include(../Assets.cmake)

add_dependencies(${PROJECT_NAME} Assets)