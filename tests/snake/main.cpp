#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/objects/RenderObject.h>
#include <source/components/AssetManager.h>
#include <source/components/PipelineManager.h>
#include <source/pipelines/custom/config/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void populateLights(const vke::VulkanEngine& renderer, std::vector<std::shared_ptr<vke::Light>>& lights);

constexpr vke::VulkanEngineOptions vulkanEngineOptions {
  .WINDOW_WIDTH = 800,
  .WINDOW_HEIGHT = 600,
  .WINDOW_TITLE = "Snake",
  .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
  .DO_DOTS = false
};

int main()
{
  try
  {
    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();
    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/snakeH.obj");

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<vke::Light>> lights;
    populateLights(renderer, lights);

    while (renderer.isActive())
    {
      auto x = object->getPosition().x;
      x += 0.025f;
      object->setPosition({x, 0, 0});

      displayGui(gui, lights, { object });

      // Render Objects
      renderer.getPipelineManager()->renderObject(object, vke::PipelineType::snake);

      for (const auto& light : lights)
      {
        renderer.getLightingManager()->renderLight(light);
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

void populateLights(const vke::VulkanEngine& renderer, std::vector<std::shared_ptr<vke::Light>>& lights)
{
  lights.push_back(renderer.getLightingManager()->createLight({0, 15.0f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}