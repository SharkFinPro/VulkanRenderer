#version 450

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 4) uniform Curtain {
  float amplitude;
  float period;
  float shininess;
} curtain;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

const float PI = 3.14;
const float Y0 = 5;

void main()
{
  vec3 pos = inPosition;
  pos.z = curtain.amplitude * (Y0 - pos.y) * sin ( 2. * PI * pos.x * curtain.period);
  gl_Position = transform.proj * transform.view * transform.model * vec4(pos, 1.0);

  float dzdx = curtain.amplitude * (Y0 - pos.y) * (2.0 * PI / curtain.period) * cos(2.0 * PI * pos.x / curtain.period);
  float dzdy = -curtain.amplitude * sin(2.0 * PI * pos.x / curtain.period);
  vec3 Tx = vec3(1.0, 0.0, dzdx);
  vec3 Ty = vec3(0.0, 1.0, dzdy);
  fragNormal = normalize(cross(Tx, Ty));

  fragPos = vec3(transform.model * vec4(pos, 1.0));
  fragTexCoord = inTexCoord;
}
























