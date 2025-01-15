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

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform EllipticalDots {
  float shininess;
  float sDiameter;
  float tDiameter;
  float blendFactor;
} ellipticalDots;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 OBJECTCOLOR = vec3(1, 1, 1);
const vec3 ELLIPSECOLOR = vec3(0, 0, 1);

vec3 PointLightAffect(PointLight light, vec3 color)
{
  vec3 Normal = normalize(fragNormal);
  vec3 Light = normalize(light.position);
  vec3 Eye = normalize(camera.position);

  // Ambient
  vec3 ambient = light.ambient * color;

  // Diffuse
  float d = max(dot(Normal,Light), 0.0); // only do diffuse if the light can see the point
  vec3 diffuse = light.diffuse * d * color;

  // Specular
  float s = 0.0;
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 ref = normalize(reflect(-Light, Normal));
    float cosphi = dot(Eye, ref);
    if(cosphi > 0.0)
    {
      s = pow(max(cosphi, 0.0), ellipticalDots.shininess);
    }
  }
  vec3 specular = light.specular * s * light.color;

  // Combined Output
  return (ambient + diffuse + specular) * light.color;
}

void main()
{
  // blend OBJECTCOLOR and ELLIPSECOLOR by using the ellipse equation to decide how close
  // 	this fragment is to the ellipse border:

  int numins = int(fragTexCoord.s / ellipticalDots.sDiameter);
  int numint = int(fragTexCoord.t / ellipticalDots.tDiameter);

  // Calculate ellipse equation
  float Ar = ellipticalDots.sDiameter / 2.0;
  float Br = ellipticalDots.tDiameter / 2.0;

  float sc = numins * ellipticalDots.sDiameter + Ar;
  float tc = numint * ellipticalDots.tDiameter + Br;

  float dist = pow((fragTexCoord.s - sc) / Ar, 2.0) + pow((fragTexCoord.t - tc) / Br, 2.0);

  // Smooth blending based on ellipse distance
  float t = smoothstep(1.0 - ellipticalDots.blendFactor, 1.0 + ellipticalDots.blendFactor, dist);
  vec3 fragColor = mix(ELLIPSECOLOR, OBJECTCOLOR, t);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], fragColor);
  }

  outColor = vec4(result, 1.0);
}