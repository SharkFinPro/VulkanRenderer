function(install_headers)
  if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ${CMAKE_SOURCE_DIR}/source)
    #Create Include Headers
    file(COPY
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
      DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanEngine
      FILES_MATCHING
      PATTERN "*.h"
      PATTERN "include/*" EXCLUDE
    )

    message(STATUS "Headers installed to ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanEngine")
  endif()
endfunction()

install_headers()