#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>
#include <imgui.h>
#include <string>
#include <glm/gtc/type_ptr.hpp>

void displayObjectGui(const std::shared_ptr<RenderObject>& object, int id);

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

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/catH.obj");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
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


    while (renderer.isActive())
    {
      gui->dockCenter("SceneView");
      gui->dockBottom("Objects");
      gui->dockBottom("Cube Map");

      gui->setBottomDockPercent(0.42);

      // Render GUI
      ImGui::Begin("Objects");
      displayObjectGui(object, 0);
      ImGui::End();

      // Render Objects
      renderer.renderObject(object, PipelineType::cubeMap);

      renderer.renderObject(px, PipelineType::texturedPlane);
      renderer.renderObject(nx, PipelineType::texturedPlane);
      renderer.renderObject(py, PipelineType::texturedPlane);
      renderer.renderObject(ny, PipelineType::texturedPlane);
      renderer.renderObject(pz, PipelineType::texturedPlane);
      renderer.renderObject(nz, PipelineType::texturedPlane);

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

void displayObjectGui(const std::shared_ptr<RenderObject>& object, const int id)
{
  glm::vec3 position = object->getPosition();

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Object " + std::to_string(id)).c_str()))
  {
    ImGui::SliderFloat3("Position", value_ptr(position), -50.0f, 50.0f);
  }

  ImGui::PopID();

  object->setPosition(position);
}
