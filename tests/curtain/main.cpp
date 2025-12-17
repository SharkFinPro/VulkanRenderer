#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/pipelineManager/PipelineManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void renderScene(vke::VulkanEngine& renderer, const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::shared_ptr<vke::RenderObject>& object, const std::vector<std::shared_ptr<vke::Light>>& lights,
                 bool& useCurtain);

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Curtain",
      .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
      .DO_DOTS = false
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/curtain.glb");

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);

    std::vector<std::shared_ptr<vke::Light>> lights;

    lights.push_back(renderer.getLightingManager()->createPointLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    bool useCurtain = true;

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, lights, useCurtain);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void renderScene(vke::VulkanEngine& renderer, const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::shared_ptr<vke::RenderObject>& object, const std::vector<std::shared_ptr<vke::Light>>& lights,
                 bool& useCurtain)
{
  // Render GUI
  displayGui(gui, lights, { object }, renderer.getRenderingManager());

  ImGui::Begin("Rendering");
  ImGui::Checkbox("Use Curtain", &useCurtain);
  ImGui::End();


  // Render Objects
  renderer.getPipelineManager()->renderObject(object, useCurtain ? vke::PipelineType::curtain : vke::PipelineType::object);

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}