#include "../common/gui.h"
#include <source/components/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject> &object, std::vector<std::shared_ptr<RenderObject>>& walls);

void setupScene(VulkanEngine& renderer, std::shared_ptr<RenderObject>& object,
                std::vector<std::shared_ptr<RenderObject>>& walls);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Cube Map",
      .CAMERA_POSITION = { 0.0f, 0.0f, -10.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    std::shared_ptr<RenderObject> object = nullptr;
    std::vector<std::shared_ptr<RenderObject>> walls;

    setupScene(renderer, object, walls);

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, walls);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject> &object, std::vector<std::shared_ptr<RenderObject>>& walls)
{
  gui->dockCenter("SceneView");
  gui->dockBottom("Objects");
  gui->dockBottom("Cube Map");

  gui->setBottomDockPercent(0.42);

  // Render GUI
  displayObjectGuis({ object });

  // Render Objects
  renderer.renderObject(object, PipelineType::cubeMap);

  for (auto& wall : walls)
  {
    renderer.renderObject(wall, PipelineType::texturedPlane);
  }

  // Render Frame
  renderer.render();
}

void setupScene(VulkanEngine& renderer, std::shared_ptr<RenderObject>& object,
                std::vector<std::shared_ptr<RenderObject>>& walls)
{
  const auto texture = renderer.loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
  const auto model = renderer.loadModel("assets/models/catH.obj");

  object = renderer.loadRenderObject(texture, specularMap, model);
  object->setPosition({ 0, 0, -5 });
  object->setScale(2.0f);

  const auto planeModel = renderer.loadModel("assets/models/curtain.glb");

  constexpr float scale = 4.0f;
  constexpr float d = 5 * scale;
  constexpr float offset = -5.0f;

  const auto px = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvposx.bmp", false), specularMap, planeModel);
  px->setScale(scale);
  px->setPosition({ d, 0, offset });
  px->setOrientationEuler({0, 90, 0});

  const auto nx = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvnegx.bmp", false), specularMap, planeModel);
  nx->setScale(scale);
  nx->setPosition({ -d, 0, offset });
  nx->setOrientationEuler({0, -90, 0});

  const auto py = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvposy.bmp", false), specularMap, planeModel);
  py->setScale(scale);
  py->setPosition({ 0, d, offset });
  py->setOrientationEuler({-90, 0, 0});

  const auto ny = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvnegy.bmp", false), specularMap, planeModel);
  ny->setScale(scale);
  ny->setPosition({ 0, -d, offset });
  ny->setOrientationEuler({90, 0, 0});

  const auto pz = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvposz.bmp", false), specularMap, planeModel);
  pz->setScale(scale);
  pz->setPosition({ 0, 0, d + offset });
  pz->setOrientationEuler({0, 0, 0});

  const auto nz = renderer.loadRenderObject(renderer.loadTexture("assets/cubeMap/nvnegz.bmp", false), specularMap, planeModel);
  nz->setScale(scale);
  nz->setPosition({ 0, 0, -d + offset });
  nz->setOrientationEuler({0, 180, 0});

  walls.insert(walls.end(), {px, nx, py, ny, pz, nz});
}