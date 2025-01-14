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
  float ambient;
  float diffuse;
  float specular;
  float shininess;
  float sDiameter;
  float tDiameter;
  float blendFactor;
} ellipticalDots;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

//vec3 PointLightAffect(PointLight light, vec3 color)
//{
//  // ambient
//  vec3 ambient = light.ambient * color;
//
//  // diffuse
//  vec3 norm = normalize(fragNormal);
//  vec3 lightDir = normalize(light.position - fragPos);
//  float diff = max(dot(norm, lightDir), 0.0);
//  vec3 diffuse = light.diffuse * diff * color;
//
//  //
//  return (ambient + diffuse) * light.color;
//}

const vec3 OBJECTCOLOR = vec3(1, 1, 1);
const vec3 ELLIPSECOLOR = vec3(0, 0, 1);
const vec3 SPECULARCOLOR = vec3(0, 1, 0);

void main()
{
  vec3 myColor = OBJECTCOLOR;
  vec2 st = fragTexCoord;

  // blend OBJECTCOLOR and ELLIPSECOLOR by using the ellipse equation to decide how close
  // 	this fragment is to the ellipse border:

  int numins = int(st.s / ellipticalDots.sDiameter);
  int numint = int(st.t / ellipticalDots.tDiameter);

  // Calculate ellipse equation
  float Ar = ellipticalDots.sDiameter / 2.0;
  float Br = ellipticalDots.tDiameter / 2.0;

  float sc = numins * ellipticalDots.sDiameter + Ar;
  float tc = numint * ellipticalDots.tDiameter + Br;

  float dist = pow((st.s - sc) / Ar, 2.0) + pow((st.t - tc) / Br, 2.0);

  // Smooth blending based on ellipse distance
  float t = smoothstep(1.0 - ellipticalDots.blendFactor, 1.0 + ellipticalDots.blendFactor, dist);
  myColor = mix(ELLIPSECOLOR, OBJECTCOLOR, t);

  // now use myColor in the per-fragment lighting equations:

  vec3 Normal    = normalize(fragNormal);
  vec3 Light     = normalize(lights[0].position);
  vec3 Eye       = normalize(camera.position);

  vec3 ambient = ellipticalDots.ambient * myColor;

  float d = max(dot(Normal,Light), 0.0);       // only do diffuse if the light can see the point
  vec3 diffuse = ellipticalDots.diffuse * d * myColor;

  float s = 0.;
  if( d > 0.0)              // only do specular if the light can see the point
  {
    vec3 ref = normalize(  reflect( -Light, Normal )  );
    float cosphi = dot( Eye, ref );
    if( cosphi > 0. )
    s = pow( max( cosphi, 0. ), ellipticalDots.shininess );
  }
  vec3 specular = ellipticalDots.specular * s * SPECULARCOLOR.rgb;

  outColor = vec4( ambient + diffuse + specular,  1. );
}