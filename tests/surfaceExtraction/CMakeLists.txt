project("surfaceExtraction")

# Relative to this file rather than CMAKE_SOURCE_DIR, which is the consumer's root when built through FetchContent
add_executable(${PROJECT_NAME}
  main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../source/components/assets/volume/MarchingCubesExtractor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../source/components/assets/volume/VolumeChunking.cpp
)

# Header-only: glm::glm is a shared library here (BUILD_SHARED_LIBS), which the engine doesn't use either
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
