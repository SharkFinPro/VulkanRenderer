#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(push_constant) uniform PushConstants {
  float wiggle;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
  const uint object = objectSlot();

  vec3 pos = inPosition;
  pos.z += sin(pos.x * 0.5) * wiggle;

  fragPos = vec3(objects[object].model * vec4(pos, 1.0));
  fragTexCoord = inTexCoord;
  fragNormal = mat3(objects[object].normalMatrix) * inNormal;
}
