project(VulkanEngine)
include(FetchContent)

set(BUILD_SHARED_LIBS ON)

include(cmake/CompileShaders.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Sources.cmake)

add_library(${PROJECT_NAME} ${VULKAN_ENGINE_SOURCES})

# Assets
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/")

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_INCLUDE_DIRECTORIES})

# Add shader compilation dependency
add_dependencies(${PROJECT_NAME} Shaders)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ${CMAKE_SOURCE_DIR}/source)
  #Create Include Headers
  file(COPY
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanEngine
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "include/*" EXCLUDE
  )
endif()