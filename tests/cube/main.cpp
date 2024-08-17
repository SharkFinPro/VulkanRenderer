#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>

#include <imgui.h>

int main()
{
  try
  {
    auto vulkanEngineOptions = VulkanEngineOptions{};
    vulkanEngineOptions.WINDOW_WIDTH = 800;
    vulkanEngineOptions.WINDOW_HEIGHT = 600;
    vulkanEngineOptions.WINDOW_TITLE = "Cube";
    vulkanEngineOptions.cameraPosition = { 0.0f, 0.0f, -5.0f };

    VulkanEngine renderer(vulkanEngineOptions);

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/square.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    glm::vec3 position = {0, -5, 0};
    object->setPosition(position);

    while (renderer.isActive())
    {

      ImGui::Begin("Object");
      ImGui::Text("Control Position:");
      ImGui::SliderFloat("x", &position.x, -50.0f, 50.0f);
      ImGui::SliderFloat("y", &position.y, -50.0f, 50.0f);
      ImGui::SliderFloat("z", &position.z, -50.0f, 50.0f);
      ImGui::End();

      object->setPosition(position);

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