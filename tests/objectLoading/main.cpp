#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <iostream>

int main()
{
  try
  {
    const vke::EngineConfig engineConfig {
      .window {
        .width = 800,
        .height = 600,
        .title = "Object Loading",
      },
      .camera {
        .speed = 0.5f
      }
    };

    vke::VulkanEngine renderer(engineConfig);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/viking_room.png");
    const auto specular = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/viking_room.obj", { -90, 0, 0 });

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specular, model);
    object->setPosition({ 0, -1.0f, 5.0f });
    object->setScale(2.0f);

    const auto light = renderer.getLightingManager()->createPointLight({0, 3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f);

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();

    while (renderer.isActive())
    {
      displayGui(gui, { light }, { object }, renderer.getRenderingManager());

      renderer.getLightingManager()->renderLight(light);

      r3d->renderObject(object, vke::PipelineType::object);

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
