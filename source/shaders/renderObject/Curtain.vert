#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(push_constant) uniform CurtainPC {
  float amplitude;
  float period;
  float shininess;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) flat out uint fragObject;

const float PI = 3.14;
const float Y0 = 5;

void main()
{
  const uint object = objectSlot();

  vec3 pos = inPosition;
  pos.z = pc.amplitude * (Y0 - pos.y) * sin ( 2. * PI * pos.x * pc.period);
  gl_Position = scene.proj * scene.view * objects[object].model * vec4(pos, 1.0);

  float dzdx = pc.amplitude * (Y0 - pos.y) * (2.0 * PI / pc.period) * cos(2.0 * PI * pos.x / pc.period);
  float dzdy = -pc.amplitude * sin(2.0 * PI * pos.x / pc.period);
  vec3 Tx = vec3(1.0, 0.0, dzdx);
  vec3 Ty = vec3(0.0, 1.0, dzdy);
  fragNormal = normalize(cross(Tx, Ty));

  fragPos = vec3(objects[object].model * vec4(pos, 1.0));
  fragTexCoord = inTexCoord;
  fragObject = object;
}
