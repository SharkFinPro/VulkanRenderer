#include "../common/gui.h"
#include <source/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void populateLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights);

constexpr VulkanEngineOptions vulkanEngineOptions {
  .WINDOW_WIDTH = 800,
  .WINDOW_HEIGHT = 600,
  .WINDOW_TITLE = "Crosses",
  .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
  .DO_DOTS = false
};

int main()
{
  try
  {
    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();
    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/cow.obj");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<Light>> lights;
    populateLights(renderer, lights);

    while (renderer.isActive())
    {
      // Render GUI
      displayGui(gui, lights, { object });

      // Render Objects
      renderer.renderObject(object, PipelineType::crosses);

      for (const auto& light : lights)
      {
        renderer.renderLight(light);
      }

      // Render Frame
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

void populateLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights)
{
  lights.push_back(renderer.createLight({0, 15.0f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}