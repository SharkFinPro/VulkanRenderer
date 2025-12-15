#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/objects/RenderObject.h>
#include <source/components/assetManager/AssetManager.h>
#include <source/components/PipelineManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void createLights(const vke::VulkanEngine& renderer, std::vector<std::shared_ptr<vke::Light>>& lights);
void createSmokeSystems(const vke::VulkanEngine& renderer);

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Smoke",
      .CAMERA_POSITION = { 0.0f, 5.0f, -15.0f },
      .DO_DOTS = false
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<vke::Light>> lights;
    createLights(renderer, lights);

    createSmokeSystems(renderer);

    while (renderer.isActive())
    {
      displayGui(gui, lights, { object }, renderer.getRenderingManager());

      renderer.getPipelineManager()->renderObject(object, vke::PipelineType::object);

      for (const auto& light : lights)
      {
        renderer.getLightingManager()->renderLight(light);
      }

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

void createLights(const vke::VulkanEngine& renderer, std::vector<std::shared_ptr<vke::Light>>& lights)
{
  lights.push_back(renderer.getLightingManager()->createLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void createSmokeSystems(const vke::VulkanEngine& renderer)
{
  constexpr uint32_t numParticles = 2'500'000;

  renderer.getPipelineManager()->createSmokeSystem({0, 0.95f, 0}, numParticles);
  renderer.getPipelineManager()->createSmokeSystem({-5, 0.95f, -5}, numParticles * 2);
  renderer.getPipelineManager()->createSmokeSystem({-5, 0.95f, 5}, numParticles / 2);
  renderer.getPipelineManager()->createSmokeSystem({5, .95f, 5}, numParticles * 2);
  renderer.getPipelineManager()->createSmokeSystem({5, 0.95f, -5}, numParticles / 2);
}
