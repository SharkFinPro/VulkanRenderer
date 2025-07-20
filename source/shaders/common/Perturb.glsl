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

  // rotate about y:
  float xp =  n.x * cy + n.z * sy;    // x'
  n.z      = -n.x * sy + n.z * cy;    // z'
  n.x      =  xp;

  return normalize( n );
}

vec3 PerturbNormal3(float angx, float angy, float angz, vec3 n)
{
  n = PerturbNormal2(angx, angy, n);

  float cz = cos(angz);
  float sz = sin(angz);

  // rotate about z:
  float zp =  n.x * cz - n.y * sz;	// x'
  n.y      = n.x*sz + n.y * cz;	// y'
  n.x      = zp;

  return normalize( n );
}