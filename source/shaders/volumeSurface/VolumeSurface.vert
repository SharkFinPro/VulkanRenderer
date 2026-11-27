#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uvec4 inMaterials;
layout(location = 3) in vec4 inWeights;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) flat out uint fragMaterial;

void main()
{
  const uint object = objectSlot();

  fragPos = vec3(objects[object].model * vec4(inPosition, 1.0));
  fragNormal = mat3(objects[object].normalMatrix) * inNormal;
  // The vertex's own material, the one with the full weight, until surfaces blend materials
  fragMaterial = inMaterials.x;
  for (int i = 1; i < 4; i++)
  {
    if (inWeights[i] > inWeights[0])
    {
      fragMaterial = inMaterials[i];
    }
  }

  gl_Position = scene.proj * scene.view * objects[object].model * vec4(inPosition, 1.0);
}
