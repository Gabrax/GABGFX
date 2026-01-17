#ifndef GABMATH_H
#define GABMATH_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>

float DegToRad(float degrees);
float RadToDeg(float radians);
float GCD(float a, float b);
float LCM(float a, float b);
void isPrime(int val);
uint32_t PCG_Hash(uint32_t input);
float RandomFloat(uint32_t* seed);

typedef struct Vec2 { float x,y; } Vec2;
typedef struct Vec3 { float x,y,z; } Vec3;
typedef struct f4 { float x,y,z,w; } f4;

void Vec2Print(Vec2* v);
void Vec3Print(Vec3* v);

Vec2 Vec2Norm(Vec2 v);
float Vec2Len(Vec2 v);

Vec3 Vec3Add(Vec3 v1, Vec3 v2);
Vec3 Vec3Sub(Vec3 v1, Vec3 v2);
Vec3 Vec3Mul(Vec3 v1, Vec3 v2);
Vec3 Vec3MulS(Vec3 v, float scalar);
Vec3 Vec3Cross(Vec3 v1, Vec3 v2); // returns orthogonal vector
Vec3 Vec3Norm(Vec3 v);
Vec3 Vec3Reflect(Vec3 I, Vec3 N);
float Vec3Dot(Vec3 v1, Vec3 v2);
float Vec3Len(Vec3 v);
float Vec3Theta(Vec3 v1, Vec3 v2); // returns angle in radians between vectors (reverse cos)

// COLUMN-MAJOR
typedef struct Mat3 { float f[3][3]; } Mat3;
typedef struct Mat4 { float f[4][4]; } Mat4;

void MatPrint(Mat4* mat);
Mat4 MatMul(Mat4 a, Mat4 b);
Mat4 MatIdentity();
Mat4 MatPerspective(float fov_rad, float aspect_ratio, float near_plane, float far_plane);
Mat4 MatTranslate(const Mat4 mat, Vec3 position);
Mat4 MatRotateX(const Mat4 mat, float radians);
Mat4 MatRotateY(const Mat4 mat, float radians);
Mat4 MatRotateZ(const Mat4 mat, float radians);
Mat4 MatScale(const Mat4 mat, Vec3 scale);
Mat4 MatTransform(Vec3 position, Vec3 deg_rotation, Vec3 scale);
Mat4 MatInverseRT(const Mat4* m);
Mat4 MatInverse(const Mat4* m);
Mat4 MatLookAt(Vec3 position, Vec3 target, Vec3 up);

#endif // GABMATH_H

#ifdef GABMATH_IMPLEMENTATION

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
uint32_t PCG_Hash(uint32_t input)
{
  uint32_t state = input * 747796405u + 2891336453u;
  uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}
float RandomFloat(uint32_t* seed)
{
  *seed = PCG_Hash(*seed);
  return (float)(*seed) * (1.0f / 4294967296.0f); // [0,1)
}
void Vec2Print(Vec2* v)
{
  printf("[ %f %f ]\n", v->x,v->y);
}
float Vec2Len(Vec2 v)
{
  return sqrtf(v.x * v.x + v.y * v.y);
}
Vec2 Vec2Norm(Vec2 v)
{
  float len = Vec2Len(v);
  return (Vec2){ v.x / len, v.y / len};
}
void Vec3Print(Vec3* v)
{
  printf("[ %f %f %f ]\n", v->x,v->y,v->z);
}
Vec3 Vec3Add(Vec3 v1, Vec3 v2)
{
  return (Vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}
Vec3 Vec3Sub(Vec3 v1, Vec3 v2)
{
  return (Vec3){v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
}
Vec3 Vec3Mul(Vec3 v1, Vec3 v2)
{
  return (Vec3){ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}
Vec3 Vec3MulS(Vec3 v, float scalar)
{
  return (Vec3){ v.x * scalar, v.y * scalar, v.z * scalar };
}
Vec3 Vec3Cross(Vec3 v1, Vec3 v2) // Returns orthogonal vector
{
  return (Vec3){v1.y * v2.z - v1.z * v2.y,v1.z * v2.x - v1.x * v2.z,v1.x * v2.y - v1.y * v2.x};
}
float Vec3Dot(Vec3 v1, Vec3 v2)
{
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
float Vec3Len(Vec3 v)
{
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
float Vec3Theta(Vec3 v1, Vec3 v2) // returns angle in radians between vectors (reverse cos)
{
  return acosf(Vec3Dot(v1, v2) / (Vec3Len(v1) * Vec3Len(v2)));
}
Vec3 Vec3Norm(Vec3 v)
{
  float len = Vec3Len(v);
  return (Vec3){ v.x / len, v.y / len, v.z / len };
}
Vec3 Vec3Reflect(Vec3 I, Vec3 N)
{
  return Vec3Sub(I, Vec3MulS(N, 2.0f * Vec3Dot(N, I)));
}

void MatPrint(Mat4* mat)
{
  printf("[ %f %f %f %f ]\n", mat->f[0][0],mat->f[0][1],mat->f[0][2],mat->f[0][3]);
  printf("[ %f %f %f %f ]\n", mat->f[1][0],mat->f[1][1],mat->f[1][2],mat->f[1][3]);
  printf("[ %f %f %f %f ]\n", mat->f[2][0],mat->f[2][1],mat->f[2][2],mat->f[2][3]);
  printf("[ %f %f %f %f ]\n", mat->f[3][0],mat->f[3][1],mat->f[3][2],mat->f[3][3]);
}
Mat4 MatMul(Mat4 a, Mat4 b)
{
  Mat4 result = {0};

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
Mat4 MatIdentity()
{
  Mat4 mat = { 0 };
  mat.f[0][0]  = 1.0f;  
  mat.f[1][1]  = 1.0f;
  mat.f[2][2] = 1.0f;
  mat.f[3][3] = 1.0f;
  return mat;
}
Mat4 MatPerspective(float fov_rad, float aspect_ratio, float near_plane, float far_plane)
{
  Mat4 mat = { 0 };  
  mat.f[0][0]  = fov_rad / aspect_ratio;      // 1/(tan(fov/2)*aspect)
  mat.f[1][1]  = fov_rad;          // 1/tan(fov/2)
  mat.f[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
  mat.f[3][2] = -1.0f;
  mat.f[2][3] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
  mat.f[3][3] = 0.0f;
  return mat;
};
Mat4 MatTranslate(const Mat4 mat, Vec3 position)
{
  Mat4 trans = MatIdentity();
  trans.f[0][3] = position.x;
  trans.f[1][3] = position.y;
  trans.f[2][3] = position.z;
  return MatMul(mat, trans);
}
Mat4 MatRotateX(const Mat4 mat, float radians)
{
  Mat4 rot = MatIdentity();
  rot.f[1][1] = cosf(radians); rot.f[1][2] = -sinf(radians);
  rot.f[2][1] = sinf(radians); rot.f[2][2] = cosf(radians);
  return MatMul(mat, rot);
}
Mat4 MatRotateY(const Mat4 mat, float radians)
{
  Mat4 rot = MatIdentity();
  rot.f[0][0] = cosf(radians);  rot.f[0][2] = sinf(radians);
  rot.f[2][0] = -sinf(radians); rot.f[2][2] = cosf(radians);
  return MatMul(mat, rot);
}
Mat4 MatRotateZ(const Mat4 mat, float radians)
{
  Mat4 rot = MatIdentity();
  rot.f[0][0] = cosf(radians); rot.f[0][1] = -sinf(radians);
  rot.f[1][0] = sinf(radians); rot.f[1][1] = cosf(radians);
  return MatMul(mat, rot);
}
Mat4 MatScale(const Mat4 mat, Vec3 scale)
{
  Mat4 s = MatIdentity();
  s.f[0][0] = scale.x;
  s.f[1][1] = scale.y;
  s.f[2][2] = scale.z;
  return MatMul(mat, s);
}
Mat4 MatTransform(Vec3 position, Vec3 deg_rotation, Vec3 scale)
{
  Mat4 mat = MatIdentity();

  mat = MatTranslate(mat, position);

  mat = MatScale(mat, scale);

  mat = MatRotateX(mat, DegToRad(deg_rotation.x));
  mat = MatRotateY(mat, DegToRad(deg_rotation.y));
  mat = MatRotateZ(mat, DegToRad(deg_rotation.z));

  return mat;
}
Mat4 MatInverseRT(const Mat4* m)
{
  Mat4 inv = {0};

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
Mat4 MatInverse(const Mat4* m)
{
    Mat4 inv;
    float det;

    inv.f[0][0] =  m->f[1][1] * m->f[2][2] * m->f[3][3] -
                   m->f[1][1] * m->f[2][3] * m->f[3][2] -
                   m->f[2][1] * m->f[1][2] * m->f[3][3] +
                   m->f[2][1] * m->f[1][3] * m->f[3][2] +
                   m->f[3][1] * m->f[1][2] * m->f[2][3] -
                   m->f[3][1] * m->f[1][3] * m->f[2][2];

    inv.f[0][1] = -m->f[0][1] * m->f[2][2] * m->f[3][3] +
                   m->f[0][1] * m->f[2][3] * m->f[3][2] +
                   m->f[2][1] * m->f[0][2] * m->f[3][3] -
                   m->f[2][1] * m->f[0][3] * m->f[3][2] -
                   m->f[3][1] * m->f[0][2] * m->f[2][3] +
                   m->f[3][1] * m->f[0][3] * m->f[2][2];

    inv.f[0][2] =  m->f[0][1] * m->f[1][2] * m->f[3][3] -
                   m->f[0][1] * m->f[1][3] * m->f[3][2] -
                   m->f[1][1] * m->f[0][2] * m->f[3][3] +
                   m->f[1][1] * m->f[0][3] * m->f[3][2] +
                   m->f[3][1] * m->f[0][2] * m->f[1][3] -
                   m->f[3][1] * m->f[0][3] * m->f[1][2];

    inv.f[0][3] = -m->f[0][1] * m->f[1][2] * m->f[2][3] +
                   m->f[0][1] * m->f[1][3] * m->f[2][2] +
                   m->f[1][1] * m->f[0][2] * m->f[2][3] -
                   m->f[1][1] * m->f[0][3] * m->f[2][2] -
                   m->f[2][1] * m->f[0][2] * m->f[1][3] +
                   m->f[2][1] * m->f[0][3] * m->f[1][2];

    inv.f[1][0] = -m->f[1][0] * m->f[2][2] * m->f[3][3] +
                   m->f[1][0] * m->f[2][3] * m->f[3][2] +
                   m->f[2][0] * m->f[1][2] * m->f[3][3] -
                   m->f[2][0] * m->f[1][3] * m->f[3][2] -
                   m->f[3][0] * m->f[1][2] * m->f[2][3] +
                   m->f[3][0] * m->f[1][3] * m->f[2][2];

    inv.f[1][1] =  m->f[0][0] * m->f[2][2] * m->f[3][3] -
                   m->f[0][0] * m->f[2][3] * m->f[3][2] -
                   m->f[2][0] * m->f[0][2] * m->f[3][3] +
                   m->f[2][0] * m->f[0][3] * m->f[3][2] +
                   m->f[3][0] * m->f[0][2] * m->f[2][3] -
                   m->f[3][0] * m->f[0][3] * m->f[2][2];

    inv.f[1][2] = -m->f[0][0] * m->f[1][2] * m->f[3][3] +
                   m->f[0][0] * m->f[1][3] * m->f[3][2] +
                   m->f[1][0] * m->f[0][2] * m->f[3][3] -
                   m->f[1][0] * m->f[0][3] * m->f[3][2] -
                   m->f[3][0] * m->f[0][2] * m->f[1][3] +
                   m->f[3][0] * m->f[0][3] * m->f[1][2];

    inv.f[1][3] =  m->f[0][0] * m->f[1][2] * m->f[2][3] -
                   m->f[0][0] * m->f[1][3] * m->f[2][2] -
                   m->f[1][0] * m->f[0][2] * m->f[2][3] +
                   m->f[1][0] * m->f[0][3] * m->f[2][2] +
                   m->f[2][0] * m->f[0][2] * m->f[1][3] -
                   m->f[2][0] * m->f[0][3] * m->f[1][2];

    inv.f[2][0] =  m->f[1][0] * m->f[2][1] * m->f[3][3] -
                   m->f[1][0] * m->f[2][3] * m->f[3][1] -
                   m->f[2][0] * m->f[1][1] * m->f[3][3] +
                   m->f[2][0] * m->f[1][3] * m->f[3][1] +
                   m->f[3][0] * m->f[1][1] * m->f[2][3] -
                   m->f[3][0] * m->f[1][3] * m->f[2][1];

    inv.f[2][1] = -m->f[0][0] * m->f[2][1] * m->f[3][3] +
                   m->f[0][0] * m->f[2][3] * m->f[3][1] +
                   m->f[2][0] * m->f[0][1] * m->f[3][3] -
                   m->f[2][0] * m->f[0][3] * m->f[3][1] -
                   m->f[3][0] * m->f[0][1] * m->f[2][3] +
                   m->f[3][0] * m->f[0][3] * m->f[2][1];

    inv.f[2][2] =  m->f[0][0] * m->f[1][1] * m->f[3][3] -
                   m->f[0][0] * m->f[1][3] * m->f[3][1] -
                   m->f[1][0] * m->f[0][1] * m->f[3][3] +
                   m->f[1][0] * m->f[0][3] * m->f[3][1] +
                   m->f[3][0] * m->f[0][1] * m->f[1][3] -
                   m->f[3][0] * m->f[0][3] * m->f[1][1];

    inv.f[2][3] = -m->f[0][0] * m->f[1][1] * m->f[2][3] +
                   m->f[0][0] * m->f[1][3] * m->f[2][1] +
                   m->f[1][0] * m->f[0][1] * m->f[2][3] -
                   m->f[1][0] * m->f[0][3] * m->f[2][1] -
                   m->f[2][0] * m->f[0][1] * m->f[1][3] +
                   m->f[2][0] * m->f[0][3] * m->f[1][1];

    inv.f[3][0] = -m->f[1][0] * m->f[2][1] * m->f[3][2] +
                   m->f[1][0] * m->f[2][2] * m->f[3][1] +
                   m->f[2][0] * m->f[1][1] * m->f[3][2] -
                   m->f[2][0] * m->f[1][2] * m->f[3][1] -
                   m->f[3][0] * m->f[1][1] * m->f[2][2] +
                   m->f[3][0] * m->f[1][2] * m->f[2][1];

    inv.f[3][1] =  m->f[0][0] * m->f[2][1] * m->f[3][2] -
                   m->f[0][0] * m->f[2][2] * m->f[3][1] -
                   m->f[2][0] * m->f[0][1] * m->f[3][2] +
                   m->f[2][0] * m->f[0][2] * m->f[3][1] +
                   m->f[3][0] * m->f[0][1] * m->f[2][2] -
                   m->f[3][0] * m->f[0][2] * m->f[2][1];

    inv.f[3][2] = -m->f[0][0] * m->f[1][1] * m->f[3][2] +
                   m->f[0][0] * m->f[1][2] * m->f[3][1] +
                   m->f[1][0] * m->f[0][1] * m->f[3][2] -
                   m->f[1][0] * m->f[0][2] * m->f[3][1] -
                   m->f[3][0] * m->f[0][1] * m->f[1][2] +
                   m->f[3][0] * m->f[0][2] * m->f[1][1];

    inv.f[3][3] =  m->f[0][0] * m->f[1][1] * m->f[2][2] -
                   m->f[0][0] * m->f[1][2] * m->f[2][1] -
                   m->f[1][0] * m->f[0][1] * m->f[2][2] +
                   m->f[1][0] * m->f[0][2] * m->f[2][1] +
                   m->f[2][0] * m->f[0][1] * m->f[1][2] -
                   m->f[2][0] * m->f[0][2] * m->f[1][1];

    det = m->f[0][0] * inv.f[0][0] +
          m->f[0][1] * inv.f[1][0] +
          m->f[0][2] * inv.f[2][0] +
          m->f[0][3] * inv.f[3][0];

    if (det == 0.0f) {
        // macierz osobliwa — brak odwrotności
        return (Mat4){0};
    }

    det = 1.0f / det;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            inv.f[i][j] *= det;

    return inv;
}
Mat4 MatLookAt(Vec3 position, Vec3 target, Vec3 up)
{
  Vec3 f = Vec3Norm(Vec3Sub(target, position));  // forward
  Vec3 s = Vec3Norm(Vec3Cross(up, f));           // right (swapped order)
  Vec3 u = Vec3Cross(f, s);                      // true up

  Mat4 mat = MatIdentity();

  // Rotation part (basis vectors)
  mat.f[0][0] = s.x;  mat.f[0][1] = s.y;  mat.f[0][2] = s.z;  mat.f[0][3] = -Vec3Dot(s, position);
  mat.f[1][0] = u.x;  mat.f[1][1] = u.y;  mat.f[1][2] = u.z;  mat.f[1][3] = -Vec3Dot(u, position);
  mat.f[2][0] = -f.x; mat.f[2][1] = -f.y; mat.f[2][2] = -f.z; mat.f[2][3] =  Vec3Dot(f, position);
  mat.f[3][0] = 0.0f; mat.f[3][1] = 0.0f; mat.f[3][2] = 0.0f; mat.f[3][3] = 1.0f;

  return mat;
}
#endif // MYLIB_IMPLEMENTATION
