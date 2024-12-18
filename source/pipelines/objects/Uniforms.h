#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>
#include <imgui.h>
#include <string>
#include <array>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct alignas(16) Light {
  std::array<float, 3> position;
  float padding1;
  std::array<float, 3> color;
  float padding2;

  float ambient;
  float diffuse;
  float specular;
  float padding3;

  void displayGui(const size_t lightNum)
  {
    ImGui::PushID(static_cast<int>(lightNum));

    if (ImGui::CollapsingHeader(("Light " + std::to_string(lightNum)).c_str()))
    {
      ImGui::ColorEdit3("Color", color.data());
      ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
      ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
      ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f);
      ImGui::SliderFloat3("Position", position.data(), -50.0f, 50.0f);
      ImGui::Separator();
    }

    ImGui::PopID();
  }
};

struct LightMetadataUniform {
  alignas(16) int numLights;
};

typedef Light* LightsUniform;

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

#endif //VULKANPROJECT_UNIFORMS_H
