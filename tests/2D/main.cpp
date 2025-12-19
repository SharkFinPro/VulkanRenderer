#include "../common/gui.h"
#include <source/VulkanEngine.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/renderingManager/renderer2D/Renderer2D.h>
#include <imgui.h>
#include <iostream>


void renderScene(vke::VulkanEngine& renderer, const std::shared_ptr<vke::ImGuiInstance>& gui);

int main()
{
  try
  {
    constexpr vke::VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "2D",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f },
      .DO_DOTS = false
    };

    vke::VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto r2d = renderer.getRenderingManager()->getRenderer2D();

    const auto assetManager = renderer.getAssetManager();
    const auto font = assetManager->loadFont("assets/fonts/Roboto-VariableFont_wdth,wght.ttf", 32);
    r2d->font(font);

    while (renderer.isActive())
    {

      r2d->fill(200, 100, 50);
      r2d->pushMatrix();
      r2d->translate(150, 150);
      r2d->rotate(45.0f);
      r2d->rect(-50, -50, 100, 100);
      r2d->popMatrix();

      r2d->fill(50, 100, 200);
      r2d->rect(250, 100, 100, 100);

      r2d->text("Hello, World!", 400, 200);

      renderScene(renderer, gui);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void renderScene(vke::VulkanEngine& renderer, const std::shared_ptr<vke::ImGuiInstance>& gui)
{
  // Render GUI
  displayGui(gui, {}, {}, renderer.getRenderingManager());

  // Render Frame
  renderer.render();
}