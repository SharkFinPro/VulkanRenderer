#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(location = 0) in vec3 inPosition;

layout(location = 0) flat out uint pickId;

void main()
{
  const uint object = uint(gl_InstanceIndex);

  pickId = objects[object].pickId;

  gl_Position = scene.proj * scene.view * objects[object].model * vec4(inPosition, 1.0);
}
