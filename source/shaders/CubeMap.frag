#version 450

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform CubeMap {
  float mix;
  float refractionIndex;
  float whiteMix;
} cubeMap;

layout(set = 0, binding = 6) uniform NoiseOptions {
  float amplitude;
  float frequency;
} noiseOptions;

layout(set = 0, binding = 7) uniform sampler3D Noise3;

layout(set = 0, binding = 8) uniform samplerCube ReflectUnit;
layout(set = 0, binding = 9) uniform samplerCube RefractUnit;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 WHITE = vec3(1.0, 1.0, 1.0);

vec3 PerturbNormal3(float angx, float angy, float angz, vec3 n)
{
  float cx = cos( angx );
  float sx = sin( angx );
  float cy = cos( angy );
  float sy = sin( angy );
  float cz = cos( angz );
  float sz = sin( angz );

  // rotate about x:
  float yp =  n.y*cx - n.z*sx;	// y'
  n.z      =  n.y*sx + n.z*cx;	// z'
  n.y      =  yp;
  // n.x      =  n.x;

  // rotate about y:
  float xp =  n.x*cy + n.z*sy;	// x'
  n.z      = -n.x*sy + n.z*cy;	// z'
  n.x      =  xp;
  // n.y      =  n.y;

  // rotate about z:
  xp =  n.x*cz - n.y*sz;	// x'
  n.y      =  n.x*sz + n.y*cz;	// y'
  n.x      = xp;
  // n.z      =  n.z;

  return normalize( n );
}

void main()
{
  vec3 Normal = normalize(fragNormal);
  vec3 Eye = normalize(fragPos - camera.position);

  vec4 nvx = texture(Noise3, noiseOptions.frequency * fragPos);
  vec4 nvy = texture(Noise3, noiseOptions.frequency * vec3(fragPos.xy, fragPos.z + 0.33));
  vec4 nvz = texture(Noise3, noiseOptions.frequency * vec3(fragPos.xy, fragPos.z + 0.67));

  float angx = nvx.r + nvx.g + nvx.b + nvx.a;	//  1. -> 3.
  angx = angx - 2.;				// -1. -> 1.
  angx *= noiseOptions.amplitude;

  float angy = nvy.r + nvy.g + nvy.b + nvy.a;	//  1. -> 3.
  angy = angy - 2.;				// -1. -> 1.
  angy *= noiseOptions.amplitude;

  float angz = nvz.r + nvz.g + nvz.b + nvz.a;	//  1. -> 3.
  angz = angz - 2.;				// -1. -> 1.
  angz *= noiseOptions.amplitude;

  Normal = PerturbNormal3( angx, angy, angz, Normal );
  Normal = normalize(transpose(inverse(mat3(transform.model))) * Normal);

  vec3 reflectVector = reflect(Eye, Normal);
  vec3 reflectColor = texture(ReflectUnit, reflectVector).rgb;

  vec3 refractVector = refract(Eye, Normal, cubeMap.refractionIndex);

  vec3 refractColor;
  if( all( equal( refractVector, vec3(0.,0.,0.) ) ) )
  {
    refractColor = reflectColor;
  }
  else
  {
    refractColor = texture( RefractUnit, refractVector ).rgb;
    refractColor = mix( refractColor, WHITE, cubeMap.whiteMix );
  }

  outColor = vec4(mix(refractColor, reflectColor, cubeMap.mix), 1);
}