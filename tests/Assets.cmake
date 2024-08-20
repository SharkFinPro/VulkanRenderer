add_custom_target(Assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/../assets"
  "${CMAKE_BINARY_DIR}/bin/assets"
  COMMENT "Copying Assets"
)

add_dependencies(${PROJECT_NAME} Assets)