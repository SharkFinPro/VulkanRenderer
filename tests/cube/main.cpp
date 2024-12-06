#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>

#include <imgui.h>

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Cube",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f },
      .DO_DOTS = true
    };

    VulkanEngine renderer(vulkanEngineOptions);

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/square.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    glm::vec3 position = {0, -5, 0};
    object->setPosition(position);

    renderer.createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f);

    renderer.createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f);

    renderer.createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f);

    renderer.createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f);

    renderer.createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f);

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