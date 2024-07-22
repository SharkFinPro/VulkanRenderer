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
    vulkanEngineOptions.WINDOW_TITLE = "Object Loading";
    vulkanEngineOptions.VERTEX_SHADER_FILE = "assets/shaders/vert.spv";
    vulkanEngineOptions.FRAGMENT_SHADER_FILE = "assets/shaders/frag.spv";

    VulkanEngine renderer(vulkanEngineOptions);

    auto texture = renderer.loadTexture("assets/textures/viking_room.png");
    auto specular = renderer.loadTexture("assets/textures/blank_specular.png");
    auto model = renderer.loadModel("assets/models/viking_room.obj");

    auto object = renderer.loadRenderObject(texture, specular, model);
    object->setPosition({ 0, -0.5f, 3.0f });


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