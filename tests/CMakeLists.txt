set(TESTS
  2D
  clouds
  crosses
  cubeMap
  magicLens
  manyObjects
  mousePicking
  objectLoading
  plants
  renderObject
  shadows
  smoke
  snake
)

foreach(test ${TESTS})
  add_subdirectory(${test})

  # Compile against the engine's precompiled header instead of re-parsing Vulkan-Hpp. Reuse needs the library's
  # exact compile options, so the tests take its PIC flag (POSITION_INDEPENDENT_CODE would give executables -fPIE).
  target_compile_options(${test} PRIVATE ${CMAKE_CXX_COMPILE_OPTIONS_PIC})
  target_precompile_headers(${test} REUSE_FROM VulkanEngine)
endforeach()

# Create a custom target that builds all projects
add_custom_target(build_all
  DEPENDS ${TESTS}
  COMMENT "Building all tests"
)

# Headless checks: no engine, no GPU. They compile the sources they check instead of linking VulkanEngine, whose
# shared library needs the Vulkan loader at load time, and run under ctest.
add_subdirectory(cullingMath)
add_test(NAME cullingMath COMMAND cullingMath)
add_dependencies(build_all cullingMath)
