#ifndef GABMATH_H
#define GABMATH_H

float DegToRad(float degrees);
float RadToDeg(float radians);
float GCD(float a, float b);
float LCM(float a, float b);
void isPrime(int val);

typedef struct f2 { float x,y; } f2;
typedef struct f3 { float x,y,z; } f3;
typedef struct f4 { float x,y,z,w; } f4;

void f2Print(f2* v);
void f3Print(f3* v);

f2 f2Norm(f2 v);
float f2Len(f2 v);

f3 f3Add(f3 v1, f3 v2);
f3 f3Sub(f3 v1, f3 v2);
f3 f3Mul(f3 v1, f3 v2);
f3 f3MulS(f3 v, float scalar);
f3 f3Cross(f3 v1, f3 v2); // returns orthogonal vector
f3 f3Norm(f3 v);
f3 f3Reflect(f3 I, f3 N);
float f3Dot(f3 v1, f3 v2);
float f3Len(f3 v);
float f3Theta(f3 v1, f3 v2); // returns angle in radians between vectors (reverse cos)

// COLUMN-MAJOR
typedef struct f3x3 { float f[3][3]; } f3x3;
typedef struct f4x4 { float f[4][4]; } f4x4;

void MatPrint(f4x4* mat);
f4x4 MatMul(f4x4 a, f4x4 b);
f4x4 MatIdentity();
f4x4 MatPerspective(float fov_rad, float aspect_ratio, float near_plane, float far_plane);
f4x4 MatTranslate(const f4x4 mat, f3 position);
f4x4 MatRotateX(const f4x4 mat, float radians);
f4x4 MatRotateY(const f4x4 mat, float radians);
f4x4 MatRotateZ(const f4x4 mat, float radians);
f4x4 MatScale(const f4x4 mat, f3 scale);
f4x4 MatTransform(f3 position, f3 deg_rotation, f3 scale);
f4x4 MatInverseRT(const f4x4* m);
f4x4 MatLookAt(f3 position, f3 target, f3 up);

#endif // GABMATH_H

#ifdef GABMATH_IMPLEMENTATION
#include <stdio.h>
#include <math.h>

float DegToRad(float degrees)
{
  return degrees * (3.14159265358979323846f / 180.0f);
}
float RadToDeg(float radians)
{
  return radians * (180.0f / 3.14159265358979323846f);
}
float GCD(float a, float b)
{
  while(a != b)
  {
    if(a > b) a -= b;
    else b-= a;
  }
  return a;
}
float LCM(float a, float b)
{
 return fabsf(a * b) / GCD(a,b);
}
void isPrime(int val)
{
  int count = 0;
  for(int i=1;i<=val;++i) 
    if((val % i) == 0) count++;

  if(count == 2) printf("%d ", val);
}

void f2Print(f2* v)
{
  printf("[ %f %f ]\n", v->x,v->y);
}
float f2Len(f2 v)
{
  return sqrtf(v.x * v.x + v.y * v.y);
}
f2 f2Norm(f2 v)
{
  float len = f2Len(v);
  return (f2){ v.x / len, v.y / len};
}
void f3Print(f3* v)
{
  printf("[ %f %f %f ]\n", v->x,v->y,v->z);
}
f3 f3Add(f3 v1, f3 v2)
{
  return (f3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}
f3 f3Sub(f3 v1, f3 v2)
{
  return (f3){v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
}
f3 f3Mul(f3 v1, f3 v2)
{
  return (f3){ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}
f3 f3MulS(f3 v, float scalar)
{
  return (f3){ v.x * scalar, v.y * scalar, v.z * scalar };
}
f3 f3Cross(f3 v1, f3 v2) // Returns orthogonal vector
{
  return (f3){v1.y * v2.z - v1.z * v2.y,v1.z * v2.x - v1.x * v2.z,v1.x * v2.y - v1.y * v2.x};
}
float f3Dot(f3 v1, f3 v2)
{
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
float f3Len(f3 v)
{
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
float f3Theta(f3 v1, f3 v2) // returns angle in radians between vectors (reverse cos)
{
  return acosf(f3Dot(v1, v2) / (f3Len(v1) * f3Len(v2)));
}
f3 f3Norm(f3 v)
{
  float len = f3Len(v);
  return (f3){ v.x / len, v.y / len, v.z / len };
}
f3 f3Reflect(f3 I, f3 N)
{
  return f3Sub(I, f3MulS(N, 2.0f * f3Dot(N, I)));
}

void MatPrint(f4x4* mat)
{
  printf("[ %f %f %f %f ]\n", mat->f[0][0],mat->f[0][1],mat->f[0][2],mat->f[0][3]);
  printf("[ %f %f %f %f ]\n", mat->f[1][0],mat->f[1][1],mat->f[1][2],mat->f[1][3]);
  printf("[ %f %f %f %f ]\n", mat->f[2][0],mat->f[2][1],mat->f[2][2],mat->f[2][3]);
  printf("[ %f %f %f %f ]\n", mat->f[3][0],mat->f[3][1],mat->f[3][2],mat->f[3][3]);
}
f4x4 MatMul(f4x4 a, f4x4 b)
{
  f4x4 result = {0};

  // Row 0
  result.f[0][0] = a.f[0][0]*b.f[0][0] + a.f[0][1]*b.f[1][0] + a.f[0][2]*b.f[2][0] + a.f[0][3]*b.f[3][0];
  result.f[0][1] = a.f[0][0]*b.f[0][1] + a.f[0][1]*b.f[1][1] + a.f[0][2]*b.f[2][1] + a.f[0][3]*b.f[3][1];
  result.f[0][2] = a.f[0][0]*b.f[0][2] + a.f[0][1]*b.f[1][2] + a.f[0][2]*b.f[2][2] + a.f[0][3]*b.f[3][2];
  result.f[0][3] = a.f[0][0]*b.f[0][3] + a.f[0][1]*b.f[1][3] + a.f[0][2]*b.f[2][3] + a.f[0][3]*b.f[3][3];

  // Row 1
  result.f[1][0] = a.f[1][0]*b.f[0][0] + a.f[1][1]*b.f[1][0] + a.f[1][2]*b.f[2][0] + a.f[1][3]*b.f[3][0];
  result.f[1][1] = a.f[1][0]*b.f[0][1] + a.f[1][1]*b.f[1][1] + a.f[1][2]*b.f[2][1] + a.f[1][3]*b.f[3][1];
  result.f[1][2] = a.f[1][0]*b.f[0][2] + a.f[1][1]*b.f[1][2] + a.f[1][2]*b.f[2][2] + a.f[1][3]*b.f[3][2];
  result.f[1][3] = a.f[1][0]*b.f[0][3] + a.f[1][1]*b.f[1][3] + a.f[1][2]*b.f[2][3] + a.f[1][3]*b.f[3][3];

  // Row 2
  result.f[2][0] = a.f[2][0]*b.f[0][0] + a.f[2][1]*b.f[1][0] + a.f[2][2]*b.f[2][0] + a.f[2][3]*b.f[3][0];
  result.f[2][1] = a.f[2][0]*b.f[0][1] + a.f[2][1]*b.f[1][1] + a.f[2][2]*b.f[2][1] + a.f[2][3]*b.f[3][1];
  result.f[2][2] = a.f[2][0]*b.f[0][2] + a.f[2][1]*b.f[1][2] + a.f[2][2]*b.f[2][2] + a.f[2][3]*b.f[3][2];
  result.f[2][3] = a.f[2][0]*b.f[0][3] + a.f[2][1]*b.f[1][3] + a.f[2][2]*b.f[2][3] + a.f[2][3]*b.f[3][3];

  // Row 3
  result.f[3][0] = a.f[3][0]*b.f[0][0] + a.f[3][1]*b.f[1][0] + a.f[3][2]*b.f[2][0] + a.f[3][3]*b.f[3][0];
  result.f[3][1] = a.f[3][0]*b.f[0][1] + a.f[3][1]*b.f[1][1] + a.f[3][2]*b.f[2][1] + a.f[3][3]*b.f[3][1];
  result.f[3][2] = a.f[3][0]*b.f[0][2] + a.f[3][1]*b.f[1][2] + a.f[3][2]*b.f[2][2] + a.f[3][3]*b.f[3][2];
  result.f[3][3] = a.f[3][0]*b.f[0][3] + a.f[3][1]*b.f[1][3] + a.f[3][2]*b.f[2][3] + a.f[3][3]*b.f[3][3];

  return result;
}
f4x4 MatIdentity()
{
  f4x4 mat = { 0 };
  mat.f[0][0]  = 1.0f;  
  mat.f[1][1]  = 1.0f;
  mat.f[2][2] = 1.0f;
  mat.f[3][3] = 1.0f;
  return mat;
}
f4x4 MatPerspective(float fov_rad, float aspect_ratio, float near_plane, float far_plane)
{
  f4x4 mat = { 0 };  
  mat.f[0][0]  = fov_rad / aspect_ratio;      // 1/(tan(fov/2)*aspect)
  mat.f[1][1]  = fov_rad;          // 1/tan(fov/2)
  mat.f[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
  mat.f[3][2] = -1.0f;
  mat.f[2][3] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
  mat.f[3][3] = 0.0f;
  return mat;
};
f4x4 MatTranslate(const f4x4 mat, f3 position)
{
  f4x4 trans = MatIdentity();
  trans.f[0][3] = position.x;
  trans.f[1][3] = position.y;
  trans.f[2][3] = position.z;
  return MatMul(mat, trans);
}
f4x4 MatRotateX(const f4x4 mat, float radians)
{
  f4x4 rot = MatIdentity();
  rot.f[1][1] = cosf(radians); rot.f[1][2] = -sinf(radians);
  rot.f[2][1] = sinf(radians); rot.f[2][2] = cosf(radians);
  return MatMul(mat, rot);
}
f4x4 MatRotateY(const f4x4 mat, float radians)
{
  f4x4 rot = MatIdentity();
  rot.f[0][0] = cosf(radians);  rot.f[0][2] = sinf(radians);
  rot.f[2][0] = -sinf(radians); rot.f[2][2] = cosf(radians);
  return MatMul(mat, rot);
}
f4x4 MatRotateZ(const f4x4 mat, float radians)
{
  f4x4 rot = MatIdentity();
  rot.f[0][0] = cosf(radians); rot.f[0][1] = -sinf(radians);
  rot.f[1][0] = sinf(radians); rot.f[1][1] = cosf(radians);
  return MatMul(mat, rot);
}
f4x4 MatScale(const f4x4 mat, f3 scale)
{
  f4x4 s = MatIdentity();
  s.f[0][0] = scale.x;
  s.f[1][1] = scale.y;
  s.f[2][2] = scale.z;
  return MatMul(mat, s);
}
f4x4 MatTransform(f3 position, f3 deg_rotation, f3 scale)
{
  f4x4 mat = MatIdentity();

  mat = MatTranslate(mat, position);

  mat = MatScale(mat, scale);

  mat = MatRotateX(mat, DegToRad(deg_rotation.x));
  mat = MatRotateY(mat, DegToRad(deg_rotation.y));
  mat = MatRotateZ(mat, DegToRad(deg_rotation.z));

  return mat;
}
f4x4 MatInverseRT(const f4x4* m)
{
  f4x4 inv = {0};

  // transpose rotation part
  inv.f[0][0] = m->f[0][0]; inv.f[0][1] = m->f[1][0]; inv.f[0][2] = m->f[2][0];
  inv.f[1][0] = m->f[0][1]; inv.f[1][1] = m->f[1][1]; inv.f[1][2] = m->f[2][1];
  inv.f[2][0] = m->f[0][2]; inv.f[2][1] = m->f[1][2]; inv.f[2][2] = m->f[2][2];

  // inverse translation
  inv.f[0][3] = -(inv.f[0][0] * m->f[0][3] + inv.f[0][1] * m->f[1][3] + inv.f[0][2] * m->f[2][3]);
  inv.f[1][3] = -(inv.f[1][0] * m->f[0][3] + inv.f[1][1] * m->f[1][3] + inv.f[1][2] * m->f[2][3]);
  inv.f[2][3] = -(inv.f[2][0] * m->f[0][3] + inv.f[2][1] * m->f[1][3] + inv.f[2][2] * m->f[2][3]);

  // bottom row
  inv.f[3][0] = 0.0f; inv.f[3][1] = 0.0f; inv.f[3][2] = 0.0f; inv.f[3][3] = 1.0f;

  return inv;
}
f4x4 MatLookAt(f3 position, f3 target, f3 up)
{
  f3 f = f3Norm(f3Sub(target, position));  // forward
  f3 s = f3Norm(f3Cross(up, f));           // right (swapped order)
  f3 u = f3Cross(f, s);                      // true up

  f4x4 mat = MatIdentity();

  // Rotation part (basis vectors)
  mat.f[0][0] = s.x;  mat.f[0][1] = s.y;  mat.f[0][2] = s.z;  mat.f[0][3] = -f3Dot(s, position);
  mat.f[1][0] = u.x;  mat.f[1][1] = u.y;  mat.f[1][2] = u.z;  mat.f[1][3] = -f3Dot(u, position);
  mat.f[2][0] = -f.x; mat.f[2][1] = -f.y; mat.f[2][2] = -f.z; mat.f[2][3] =  f3Dot(f, position);
  mat.f[3][0] = 0.0f; mat.f[3][1] = 0.0f; mat.f[3][2] = 0.0f; mat.f[3][3] = 1.0f;

  return mat;
}
#endif // MYLIB_IMPLEMENTATION
