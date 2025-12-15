#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/objects/RenderObject.h>
#include <source/components/assetManager/AssetManager.h>
#include <source/components/PipelineManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <iostream>

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Object Loading",
      .CAMERA_SPEED = 0.5f
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/viking_room.png");
    const auto specular = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/viking_room.obj", { -90, 0, 0 });

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specular, model);
    object->setPosition({ 0, -1.0f, 5.0f });
    object->setScale(2.0f);

    const auto light = renderer.getLightingManager()->createLight({0, 3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f);

    while (renderer.isActive())
    {
      displayGui(gui, { light }, { object }, renderer.getRenderingManager());

      renderer.getLightingManager()->renderLight(light);

      renderer.getPipelineManager()->renderObject(object, vke::PipelineType::object);

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
