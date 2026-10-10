add_subdirectory(2D)
add_subdirectory(clouds)
add_subdirectory(crosses)
add_subdirectory(cubeMap)
add_subdirectory(magicLens)
add_subdirectory(mousePicking)
add_subdirectory(objectLoading)
add_subdirectory(plants)
add_subdirectory(renderObject)
add_subdirectory(shadows)
add_subdirectory(smoke)
add_subdirectory(snake)

# Compile each test against the engine's precompiled header instead of re-parsing Vulkan-Hpp. A reused header
# needs matching compile options, so the tests are built position independent like the engine library.
foreach(test 2D clouds crosses cubeMap magicLens mousePicking objectLoading plants renderObject shadows smoke snake)
  set_target_properties(${test} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_precompile_headers(${test} REUSE_FROM VulkanEngine)
endforeach()

# Create a custom target that builds all projects
add_custom_target(build_all
  DEPENDS
  2D
  clouds
  crosses
  cubeMap
  magicLens
  mousePicking
  objectLoading
  plants
  renderObject
  shadows
  smoke
  snake
  COMMENT "Building all tests"
)