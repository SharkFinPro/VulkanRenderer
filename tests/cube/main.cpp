#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>

int main()
{
  try
  {
    auto vulkanEngineOptions = VulkanEngineOptions{};
    vulkanEngineOptions.WINDOW_WIDTH = 800;
    vulkanEngineOptions.WINDOW_HEIGHT = 600;
    vulkanEngineOptions.WINDOW_TITLE = "Cube";
    vulkanEngineOptions.VERTEX_SHADER_FILE = "assets/shaders/vert.spv";
    vulkanEngineOptions.FRAGMENT_SHADER_FILE = "assets/shaders/frag.spv";
    vulkanEngineOptions.cameraPosition = { 0.0f, 0.0f, -5.0f };
    vulkanEngineOptions.cameraSpeed = 0.5f;

    VulkanEngine renderer(&vulkanEngineOptions);

    auto texture = renderer.loadTexture("assets/textures/white.png");
    auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    auto model = renderer.loadModel("assets/models/square.glb");

    auto object = renderer.loadRenderObject(texture, specularMap, model);
    object->setPosition({0.0f, -5.0f, 0.0f});

    while (renderer.isActive())
    {
      renderer.render();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}