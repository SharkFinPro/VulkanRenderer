function(copy_assets)
  set(ASSETS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/assets/")
  set(ASSETS_DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/")

  file(COPY ${ASSETS_SRC} DESTINATION ${ASSETS_DST})

  message(STATUS "Assets copied from ${ASSETS_SRC} to ${ASSETS_DST}")
endfunction()

copy_assets()