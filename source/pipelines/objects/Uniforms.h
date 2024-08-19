#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct alignas(16) Light {
  float position[3];
  float padding1;
  float color[3];
  float padding2;

  float ambient;
  float diffuse;
  float specular;
  float padding3;

  void displayGui(const size_t lightNum)
  {
    if (ImGui::CollapsingHeader(("Light " + std::to_string(lightNum) + ":").c_str()))
    {
      ImGui::SeparatorText(("Control Light " + std::to_string(lightNum)).c_str());
      ImGui::ColorEdit3(("Light " + std::to_string(lightNum) + " Color").c_str(), color);
      ImGui::SliderFloat(("Light " + std::to_string(lightNum) + " Ambient").c_str(), &ambient, 0.0f, 1.0f);
      ImGui::SliderFloat(("Light " + std::to_string(lightNum) + " Diffuse").c_str(), &diffuse, 0.0f, 1.0f);
      ImGui::SliderFloat3(("Light " + std::to_string(lightNum) + " Position").c_str(), position, -50.0f, 50.0f);
      ImGui::Separator();
    }
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
