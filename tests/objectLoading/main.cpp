#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Object Loading",
      .cameraSpeed = 0.5f
    };

    VulkanEngine renderer(vulkanEngineOptions);

    const auto texture = renderer.loadTexture("assets/textures/viking_room.png");
    const auto specular = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/viking_room.obj");

    const auto object = renderer.loadRenderObject(texture, specular, model);
    object->setPosition({ 0, -1.0f, 5.0f });
    object->setScale(2.0f);
    object->setRotation({ -90.0f, 0, 0 });

    renderer.createLight({0, 3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f);

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