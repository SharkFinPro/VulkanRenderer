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
  vec2 lensST = vec2(magnifyWhirlMosaic.lensS, magnifyWhirlMosaic.lensT);

  vec2 st = fragTexCoord - lensST;
  float r = length(st);

  if (r > magnifyWhirlMosaic.lensRadius)
  {
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;

    outColor = vec4(texColor, 1.0);
    return;
  }

  // Magnify
  float rp = r * magnifyWhirlMosaic.magnification;

  // Whirl
  float theta = atan(st.t, st.s);
  float thetap = theta - magnifyWhirlMosaic.whirl * rp;

  // Magnify + Whirl
  st = rp * vec2(cos(thetap), sin(thetap));

  // Restore Coordinates
  st += lensST;

  // Mosaic
  int numins = int(st.s / magnifyWhirlMosaic.mosaic);
  int numint = int(st.t / magnifyWhirlMosaic.mosaic);

  float m = magnifyWhirlMosaic.mosaic;
  float sc = numins * m;
  float tc = numint * m;

  st.s = sc;
  st.t = tc;

  // Sample final texture
  vec3 texColor = texture(texSampler, st).rgb;
  outColor = vec4(texColor, 1.0);
}