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

bool isCurtainPipeline(vke::PipelineType type);

std::shared_ptr<vke::RenderObject> createCubeObject(const vke::VulkanEngine& renderer);

std::shared_ptr<vke::RenderObject> createCurtainObject(const vke::VulkanEngine& renderer);

const vke::EngineConfig ENGINE_CONFIG {
  .window {
    .width = 800,
    .height = 600,
    .title = "Render Object"
  },
  .camera {
    .position = { 0.0f, 5.0f, -15.0f }
  }
};

constexpr std::array AVAILABLE_PIPELINES {
  vke::PipelineType::bumpyCurtain,
  vke::PipelineType::curtain,
  vke::PipelineType::ellipticalDots,
  vke::PipelineType::noisyEllipticalDots,
  vke::PipelineType::object
};

int main()
{
  try
  {
    vke::VulkanEngine renderer(ENGINE_CONFIG);

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto cubeObject = createCubeObject(renderer);

    const auto curtainObject = createCurtainObject(renderer);

    const auto lights = createLights(renderer);

    auto currentPipeline = vke::PipelineType::object;

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();

    while (renderer.isActive())
    {
      displayGui(renderer.getImGuiInstance(), lights, { cubeObject, curtainObject }, renderer.getRenderingManager());

      pipelineTypeGui(currentPipeline);

      if (isCurtainPipeline(currentPipeline))
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
    case vke::PipelineType::curtain: return "Curtain";
    case vke::PipelineType::ellipticalDots: return "Elliptical Dots";
    case vke::PipelineType::noisyEllipticalDots: return "Noisy Elliptical Dots";
    case vke::PipelineType::object: return "Object";
    default: return "Unknown";
  }
}

void pipelineTypeGui(vke::PipelineType& currentPipeline)
{
  ImGui::Begin("Rendering");
  if (ImGui::BeginCombo("Pipeline Type", getPipelineTypeName(currentPipeline)))
  {
    for (const auto pipeline : AVAILABLE_PIPELINES)
    {
      if (ImGui::Selectable(getPipelineTypeName(pipeline), currentPipeline == pipeline))
      {
        currentPipeline = pipeline;
      }
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

bool isCurtainPipeline(const vke::PipelineType type)
{
  return type == vke::PipelineType::curtain || type == vke::PipelineType::bumpyCurtain;
}

std::shared_ptr<vke::RenderObject> createCubeObject(const vke::VulkanEngine& renderer)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");

  const auto cubeModel = renderer.getAssetManager()->loadModel("assets/models/square.glb");

  const auto cubeObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, cubeModel);
  cubeObject->setPosition({ 0, 0, 0 });

  return cubeObject;
}

std::shared_ptr<vke::RenderObject> createCurtainObject(const vke::VulkanEngine& renderer)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");

  const auto curtainModel = renderer.getAssetManager()->loadModel("assets/models/curtain.glb");

  const auto curtainObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, curtainModel);
  curtainObject->setPosition({ 0, 0, 5 });

  return curtainObject;
}