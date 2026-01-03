add_subdirectory(2D)
add_subdirectory(crosses)
add_subdirectory(cube)
add_subdirectory(cubeMap)
add_subdirectory(grid)
add_subdirectory(magicLens)
add_subdirectory(mousePicking)
add_subdirectory(objectLoading)
add_subdirectory(plants)
add_subdirectory(renderObject)
add_subdirectory(shadows)
add_subdirectory(smoke)
add_subdirectory(snake)
add_subdirectory(spotLight)

# Create a custom target that builds all projects
add_custom_target(build_all
  DEPENDS
  2D
  crosses
  cube
  cubeMap
  grid
  magicLens
  mousePicking
  objectLoading
  plants
  renderObject
  shadows
  smoke
  snake
  spotLight
  COMMENT "Building all tests"
)