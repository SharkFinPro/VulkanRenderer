# Embeds source text and line information in the SPIR-V (glslang -gVS, the NonSemantic.Shader.DebugInfo.100
# form) so GPU profilers and capture tools such as RenderDoc and Nsight can show shader source and attribute
# cost to lines. Off by default: it makes the .spv files larger. Applications that add the engine as a
# subdirectory or with FetchContent can set it before adding it; the vke-profile preset turns it on.
option(VKE_SHADER_DEBUG_INFO "Compile shaders with debug info for GPU profilers and capture tools" OFF)

function(compile_shaders)
  set(shaderDebugFlags "")
  if (VKE_SHADER_DEBUG_INFO)
    set(shaderDebugFlags -gVS)
  endif()

  set(shadersSrc ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
  set(shadersDst ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders)

  file(MAKE_DIRECTORY ${shadersDst})

  # Find all shader files recursively
  file(GLOB_RECURSE SHADER_FILES
    "${shadersSrc}/*.vert"
    "${shadersSrc}/*.geom"
    "${shadersSrc}/*.frag"
    "${shadersSrc}/*.comp"
    "${shadersSrc}/*.rchit"
    "${shadersSrc}/*.rgen"
    "${shadersSrc}/*.rmiss"
    "${shadersSrc}/*.rint"
  )

  # Find all common shader files recursively
  file(GLOB_RECURSE COMMON_SHADER_FILES
    "${shadersSrc}/common/*.glsl"
  )

  # List to store compiled SPIR-V shader files
  set(SPV_FILES "")

  foreach(SHADER ${SHADER_FILES})
    # Get the relative path of the shader file
    file(RELATIVE_PATH REL_PATH ${shadersSrc} ${SHADER})
    get_filename_component(DIR_PATH ${REL_PATH} DIRECTORY)
    get_filename_component(FILENAME ${SHADER} NAME)

    # Set the output SPIR-V file path preserving directory structure
    set(SPV_FILE "${shadersDst}/${DIR_PATH}/${FILENAME}.spv")

    # Create the output directory if it doesn't exist
    file(MAKE_DIRECTORY "${shadersDst}/${DIR_PATH}")

    # Append to the list of SPIR-V files
    list(APPEND SPV_FILES ${SPV_FILE})

    # Add compilation command
    add_custom_command(
      OUTPUT ${SPV_FILE}
      COMMAND glslangValidator --target-env vulkan1.3 -V ${shaderDebugFlags} -I${shadersSrc}/include ${SHADER} -o ${SPV_FILE}
      DEPENDS ${SHADER} ${COMMON_SHADER_FILES}
      COMMENT "Compiling shader: ${REL_PATH}"
    )
  endforeach()

  # Define a custom target for shaders
  add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})
endfunction()

compile_shaders()