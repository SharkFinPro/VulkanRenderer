project(VulkanEngine)

add_library(${PROJECT_NAME} SHARED
  VulkanEngine.cpp
  VulkanEngine.h
  Window.cpp
  Window.h
  DebugMessenger.cpp
  DebugMessenger.h
  objects/RenderObject.cpp
  objects/RenderObject.h
  Buffers.cpp
  Buffers.h
  objects/Model.cpp
  objects/Model.h
  objects/Texture.cpp
  objects/Texture.h
  Images.cpp
  Images.h
  Camera.cpp
  Camera.h
)