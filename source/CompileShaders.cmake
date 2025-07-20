set(shadersSrc ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(shadersDst ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders)

file(MAKE_DIRECTORY ${shadersDst})

# Find all shader files recursively
file(GLOB_RECURSE SHADER_FILES
  "${shadersSrc}/*.vert"
  "${shadersSrc}/*.geom"
  "${shadersSrc}/*.frag"
  "${shadersSrc}/*.comp"
)

# List to store compiled SPIR-V shader files
set(SPV_FILES "")

foreach(SHADER ${SHADER_FILES})
  # Get the relative path of the shader file
  file(RELATIVE_PATH REL_PATH ${shadersSrc} ${SHADER})
  get_filename_component(DIR_PATH ${REL_PATH} DIRECTORY)
  get_filename_component(FILENAME ${SHADER} NAME)

  # Set the output SPIR-V file path
  set(SPV_FILE "${shadersDst}/${FILENAME}.spv")

  # Append to the list of SPIR-V files
  list(APPEND SPV_FILES ${SPV_FILE})

  # Add compilation command
  add_custom_command(
    OUTPUT ${SPV_FILE}
    COMMAND glslangValidator -V -I${shadersSrc}/include ${SHADER} -o ${SPV_FILE}
    DEPENDS ${SHADER}
    COMMENT "Compiling shader: ${REL_PATH}"
  )
endforeach()

# Define a custom target for shaders
add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})