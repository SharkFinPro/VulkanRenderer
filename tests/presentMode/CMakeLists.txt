project("presentMode")

# Relative to this file rather than CMAKE_SOURCE_DIR, which is the consumer's root when built through FetchContent
add_executable(${PROJECT_NAME}
  main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../source/components/window/PresentMode.cpp
)

# Vulkan headers only: the check never calls into the loader. Imported targets are directory-scoped, so find it here.
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Headers)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
)
