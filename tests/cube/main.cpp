#include "../common/gui.h"
#include <source/VulkanEngine.h>
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <imgui.h>
#include <iostream>

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::shared_ptr<vke::RenderObject>& object,
                 const std::vector<std::shared_ptr<vke::Light>>& lights);

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Cube",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f }
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, -5, 0 });

    std::vector<std::shared_ptr<vke::Light>> lights;

    lights.push_back(renderer.getLightingManager()->createPointLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, lights);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::shared_ptr<vke::RenderObject>& object,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

  // Render GUI
  displayGui(gui, lights, { object }, renderer.getRenderingManager());

  // Render Objects
  r3d->renderObject(object, vke::PipelineType::object);

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render lines
  r3d->renderLine({-1.0f,  0.0f, 0.0f}, {-0.5f,  0.5f, 0.0f});
  r3d->renderLine({ 0.0f,  0.0f, 0.0f}, { 0.5f, -0.5f, 0.0f});
  r3d->renderLine({ 1.0f,  0.0f, 0.0f}, { 1.5f,  0.5f, 0.0f});

  // Render Frame
  renderer.render();
}