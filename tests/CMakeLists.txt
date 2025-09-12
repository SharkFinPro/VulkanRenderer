add_subdirectory(bumpyCurtain)
add_subdirectory(crosses)
add_subdirectory(cube)
add_subdirectory(cubeMap)
add_subdirectory(curtain)
add_subdirectory(ellipticalDots)
add_subdirectory(grid)
add_subdirectory(magicLens)
add_subdirectory(mousePicking)
add_subdirectory(noisyEllipticalDots)
add_subdirectory(objectLoading)
add_subdirectory(plants)
add_subdirectory(smoke)
add_subdirectory(snake)
add_subdirectory(spotLight)

# Create a custom target that builds all projects
add_custom_target(build_all
  DEPENDS
  bumpyCurtain
  crosses
  cube
  cubeMap
  curtain
  ellipticalDots
  grid
  magicLens
  mousePicking
  noisyEllipticalDots
  objectLoading
  plants
  smoke
  snake
  spotLight
  COMMENT "Building all tests"
)