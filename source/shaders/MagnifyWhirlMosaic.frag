#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 4) uniform sampler2D specSampler;

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform MagnifyWhirlMosaic {
  float lensS;
  float lensT;
  float lensRadius;
  float magnification;
  float whirl;
  float mosaic;
} magnifyWhirlMosaic;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec2 st = fragTexCoord - vec2(magnifyWhirlMosaic.lensS, magnifyWhirlMosaic.lensT);

  if (st.x * st.x + st.y * st.y > magnifyWhirlMosaic.lensRadius * magnifyWhirlMosaic.lensRadius)
  {
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;

    outColor = vec4(texColor, 1.0);
    return;
  }

  vec3 texColor = texture(texSampler, st).rgb;

  outColor = vec4(texColor, 1.0);
}