#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <iostream>

const vke::EngineConfig ENGINE_CONFIG {
  .window {
    .width = 800,
    .height = 600,
    .title = "Render Object",
    .closeOnEscape = true
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

const char* getPipelineTypeName(vke::PipelineType type);

void pipelineTypeGui(vke::PipelineType& currentPipeline);

void linesGui(const std::shared_ptr<vke::Renderer3D>& r3d);

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer);

bool isCurtainPipeline(vke::PipelineType type);

std::shared_ptr<vke::RenderObject> createCubeObject(const vke::VulkanEngine& renderer);

std::shared_ptr<vke::RenderObject> createCurtainObject(const vke::VulkanEngine& renderer);

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

      linesGui(r3d);

      r3d->renderObject(isCurtainPipeline(currentPipeline) ? curtainObject : cubeObject, currentPipeline);

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

void linesGui(const std::shared_ptr<vke::Renderer3D>& r3d)
{
  // Above the floor and beside the cube, so depth testing doesn't hide the axes.
  constexpr glm::vec3 axesOrigin { -4.0f, 2.0f, -4.0f };
  r3d->renderLine(axesOrigin, axesOrigin + glm::vec3(3, 0, 0), { 1, 0, 0, 1 }, 3.0f);
  r3d->renderLine(axesOrigin, axesOrigin + glm::vec3(0, 3, 0), { 0, 1, 0, 1 }, 3.0f);
  r3d->renderLine(axesOrigin, axesOrigin + glm::vec3(0, 0, 3), { 0, 0, 1, 1 }, 3.0f);

  // One of each shape in a row above the floor
  constexpr float z = 5.0f;
  r3d->renderBox(glm::translate(glm::mat4(1.0f), { -7, 2, z }), glm::vec3(0.8f, 0.5f, 0.6f), { .color = { 1, 0.6f, 0, 1 } });
  r3d->renderSphere({ -4.5f, 2, z }, 0.9f, { .color = { 0, 1, 1, 1 } });
  r3d->renderCapsule({ -2, 1.2f, z }, { -2, 2.8f, z }, 0.6f, { .color = { 1, 0, 1, 1 } });
  r3d->renderCylinder({ 0, 1.2f, z }, { 0, 2.8f, z }, 0.7f, { .color = { 1, 1, 0, 1 } });
  r3d->renderCone({ 2, 3, z }, { 2, 1.2f, z }, 0.8f, { .color = { 0.5f, 1, 0.5f, 1 } });
  r3d->renderArrow({ 3.8f, 1.2f, z }, { 3.8f, 3, z }, { .color = { 1, 0.4f, 0.4f, 1 }, .width = 2.0f });
  r3d->renderAxes(glm::translate(glm::mat4(1.0f), { 5.2f, 1.2f, z }), 1.5f);
  r3d->renderFrustum(glm::perspectiveFov(glm::radians(40.0f), 4.0f, 3.0f, 0.5f, 3.0f) * glm::lookAt(glm::vec3(7, 2, z), glm::vec3(7, 2, z + 1), glm::vec3(0, 1, 0)), { .color = { 1, 1, 1, 1 } });

  // The same arrow through the floor in each depth mode, left to right as the camera sees them: tested is cut off
  // below the floor, x-ray shows that part dimmer, and on-top never hides.
  constexpr glm::vec4 arrowColor { 1, 0.4f, 0.4f, 1 };
  r3d->renderArrow({ 1.5f, -2, -1 }, { 1.5f, 2, -1 }, { .color = arrowColor, .width = 3.0f, .depth = vke::DebugDepth::tested });
  r3d->renderArrow({ 0, -2, -1 }, { 0, 2, -1 }, { .color = arrowColor, .width = 3.0f, .depth = vke::DebugDepth::xray });
  r3d->renderArrow({ -1.5f, -2, -1 }, { -1.5f, 2, -1 }, { .color = arrowColor, .width = 3.0f, .depth = vke::DebugDepth::onTop });

  ImGui::Begin("Rendering");
  if (ImGui::Button("Emit Lines (3 s)"))
  {
    for (int i = 0; i < 20; ++i)
    {
      r3d->renderLine(glm::vec3(0, 4, 0) + glm::ballRand(4.0f), glm::vec3(0, 4, 0) + glm::ballRand(4.0f), { glm::linearRand(glm::vec3(0.2f), glm::vec3(1.0f)), 0.8f },
                      glm::linearRand(1.0f, 6.0f), 3.0f);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear Lines"))
  {
    r3d->clearLines();
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