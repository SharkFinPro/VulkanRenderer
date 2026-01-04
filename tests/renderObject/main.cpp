#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

const char* getPipelineTypeName(vke::PipelineType type);

void pipelineTypeGui(vke::PipelineType& currentPipeline);

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer);

const vke::EngineConfig engineConfig {
  .window {
    .width = 800,
    .height = 600,
    .title = "Render Object"
  },
  .camera {
    .position = { 0.0f, 5.0f, -15.0f }
  }
};

int main()
{
  try
  {
    vke::VulkanEngine renderer(engineConfig);

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto gui = renderer.getImGuiInstance();

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto cubeModel = renderer.getAssetManager()->loadModel("assets/models/square.glb");
    const auto curtainModel = renderer.getAssetManager()->loadModel("assets/models/curtain.glb");

    const auto cubeObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, cubeModel);
    cubeObject->setPosition({ 0, 0, 0 });

    const auto curtainObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, curtainModel);
    curtainObject->setPosition({ 0, 0, 5 });

    const auto lights = createLights(renderer);

    auto currentPipeline = vke::PipelineType::object;

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();

    while (renderer.isActive())
    {
      displayGui(gui, lights, { cubeObject, curtainObject }, renderer.getRenderingManager());

      pipelineTypeGui(currentPipeline);

      if (currentPipeline == vke::PipelineType::curtain || currentPipeline == vke::PipelineType::bumpyCurtain)
      {
        r3d->renderObject(curtainObject, currentPipeline);
      }
      else
      {
        r3d->renderObject(cubeObject, currentPipeline);
      }

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

const char* getPipelineTypeName(const vke::PipelineType type)
{
  switch (type)
  {
    case vke::PipelineType::bumpyCurtain: return "Bumpy Curtain";
    case vke::PipelineType::crosses: return "Crosses";
    case vke::PipelineType::curtain: return "Curtain";
    case vke::PipelineType::cubeMap: return "Cube Map";
    case vke::PipelineType::ellipticalDots: return "Elliptical Dots";
    case vke::PipelineType::magnifyWhirlMosaic: return "Magnify Whirl Mosaic";
    case vke::PipelineType::noisyEllipticalDots: return "Noisy Elliptical Dots";
    case vke::PipelineType::object: return "Object";
    case vke::PipelineType::objectHighlight: return "Object Highlight";
    case vke::PipelineType::texturedPlane: return "Textured Plane";
    case vke::PipelineType::snake: return "Snake";
    default: return "Unknown";
  }
}

void pipelineTypeGui(vke::PipelineType& currentPipeline)
{
  ImGui::Begin("Rendering");
  if (ImGui::BeginCombo("Pipeline Type", getPipelineTypeName(currentPipeline)))
  {
    if (ImGui::Selectable("Bumpy Curtain", currentPipeline == vke::PipelineType::bumpyCurtain))
    {
      currentPipeline = vke::PipelineType::bumpyCurtain;
    }
    if (ImGui::Selectable("Curtain", currentPipeline == vke::PipelineType::curtain))
    {
      currentPipeline = vke::PipelineType::curtain;
    }
    if (ImGui::Selectable("Elliptical Dots", currentPipeline == vke::PipelineType::ellipticalDots))
    {
      currentPipeline = vke::PipelineType::ellipticalDots;
    }
    if (ImGui::Selectable("Noisy Elliptical Dots", currentPipeline == vke::PipelineType::noisyEllipticalDots))
    {
      currentPipeline = vke::PipelineType::noisyEllipticalDots;
    }
    if (ImGui::Selectable("Object", currentPipeline == vke::PipelineType::object))
    {
      currentPipeline = vke::PipelineType::object;
    }
    ImGui::EndCombo();
  }
  ImGui::End();
}

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer)
{
  return {
    renderer.getLightingManager()->createPointLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f)
  };
}