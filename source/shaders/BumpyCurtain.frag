#version 450

struct PointLight {
  vec3 position;
  float padding1; // Padding to ensure alignment
  vec3 color;
  float padding2; // Padding to ensure alignment
  float ambient;
  float diffuse;
  float specular;
  float padding3; // Padding to ensure alignment
};

layout(set = 1, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform Curtain {
  float amplitude;
  float period;
  float shininess;
} curtain;

layout(set = 0, binding = 6) uniform NoiseOptions {
  float amplitude;
  float frequency;
} noiseOptions;

layout(set = 0, binding = 7) uniform sampler3D Noise3;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 PointLightAffect(PointLight light, vec3 color, vec3 normal)
{
  // Ambient
  vec3 ambient = light.ambient * color;

  // Diffuse
  vec3 lightDir = normalize(light.position - fragPos);
  float d = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = light.diffuse * d * color;

  // Specular
  vec3 specular = vec3(0);
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 viewDir = normalize(camera.position - fragPos);
    vec3 reflectDir = normalize(reflect(-lightDir, normal));
    float cosphi = dot(viewDir, reflectDir);

    if (cosphi > 0.0)
    {
      specular = pow(cosphi, curtain.shininess) * light.specular * light.color;
    }
  }

  // Combined Output
  return (ambient + diffuse + specular) * light.color;
}

vec3 PerturbNormal2(float angx, float angy, vec3 n)
{
  float cx = cos(angx);
  float sx = sin(angx);
  float cy = cos(angy);
  float sy = sin(angy);

  // rotate about x:
  float yp =  n.y * cx - n.z * sx;    // y'
  n.z      =  n.y * sx + n.z * cx;    // z'
  n.y      =  yp;
  // n.x      =  n.x;

  // rotate about y:
  float xp =  n.x * cy + n.z * sy;    // x'
  n.z      = -n.x * sy + n.z * cy;    // z'
  n.x      =  xp;
  // n.y      =  n.y;

  return normalize( n );
}

void main()
{
  vec4 nvx = texture(Noise3, noiseOptions.frequency * fragPos);
  float angx = nvx.r + nvx.g + nvx.b + nvx.a  -  2.;	// -1. to +1.
  angx *= noiseOptions.amplitude;

  vec4 nvy = texture(Noise3, noiseOptions.frequency * vec3(fragPos.xy, fragPos.z + 0.5));
  float angy = nvy.r + nvy.g + nvy.b + nvy.a  -  2.;	// -1. to +1.
  angy *= noiseOptions.amplitude;

  vec3 n = PerturbNormal2(angx, angy, fragNormal);
  n = normalize(transpose(inverse(mat3(transform.model))) * n);

  vec3 fragColor = vec3(1, 1, 1);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], fragColor, n);
  }

  outColor = vec4(result, 1.0);
}