project("objectLoading")

add_executable(${PROJECT_NAME} main.cpp)

include(../Assets.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE VulkanEngine)