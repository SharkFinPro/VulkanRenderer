#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/window/Window.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/pipelineManager/PipelineManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights);

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights);

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Shadows",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f },
      .DO_DOTS = false
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    std::vector<std::shared_ptr<vke::RenderObject>> objects;
    std::vector<std::shared_ptr<vke::Light>> lights;
    setupScene(renderer, objects, lights);

    while (renderer.isActive())
    {
      renderScene(renderer, gui, objects, lights);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
  const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

  const auto object1 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object1->setPosition({ 0, -5, 0 });
  objects.push_back(object1);

  const auto object2 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object2->setPosition({ -5, -10, 0 });
  objects.push_back(object2);

  const auto object3 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object3->setPosition({ 10, 0, 15 });
  objects.push_back(object3);


  lights.push_back(renderer.getLightingManager()->createPointLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, 3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  // lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
  //
  // lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));
  //
  // lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  displayGui(gui, lights, objects, renderer.getRenderingManager());

  // Render Objects
  for (auto& object : objects)
  {
    renderer.getPipelineManager()->renderObject(object, vke::PipelineType::object);
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}