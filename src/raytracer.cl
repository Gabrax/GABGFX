
typedef struct { uchar r,g,b,a; } Color;

typedef struct Mat4 {
    float x0, x1, x2, x3;
    float y0, y1, y2, y3;
    float z0, z1, z2, z3;
    float w0, w1, w2, w3;
} Mat4;

typedef struct {
    float3 vertex[3];
    float3 normal[3];
    float2 uv[3];
    int modelIdx;
} Triangle;

typedef struct {
    int triangleOffset;
    int triangleCount;
    int vertexOffset;
    int vertexCount;
    int pixelOffset;
    int texWidth;
    int texHeight;
    Mat4 transform;
} CustomModel;

__constant float3 rayOrigin = (float3){0.0,0.0,1.0};

__constant float radius = 0.5f;

__kernel void fragment_kernel(
    __global Color* frameBuffer,
    __global float* depthBuffer,
    int width,
    int height)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  uint idx = y * width + x;

  float2 pixel_coord = (float2){ (float)x / (float)width, (float)(height - y - 1) / (float)height };
  pixel_coord.x = pixel_coord.x * 2.0f - 1.0f;
  pixel_coord.y = pixel_coord.y * 2.0f - 1.0f;

  pixel_coord.x *= (float)width / (float)height;

  float3 ray_direction = (float3){ pixel_coord.x, pixel_coord.y, -1.0 };

  float a = dot(ray_direction,ray_direction);
  float b = 2.0f * dot(rayOrigin, ray_direction);
  float c = dot(rayOrigin,rayOrigin) - radius * radius;

  float discriminant = b * b - 4.0 * a * c;

  if(discriminant >= 0.0)
  {
    float t0 = (-b + sqrt(discriminant) / (2.0 * a));
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);

    float3 h0 = rayOrigin + ray_direction * t0;
    float3 h1 = rayOrigin + ray_direction * t1;

    h1 = normalize(h1);

    float3 lightDir = (float3){-1.0f,-1.0f,-1.0f};
    lightDir = normalize(lightDir);

    float d = max(dot(h1,-lightDir), 0.0f);

    frameBuffer[idx] = (Color){
        (uchar)(1 * d * 255.0),
        (uchar)(0 * d * 255.0),
        (uchar)(1 * d * 255.0),
        255
    };
  }
  else 
  {
    frameBuffer[idx] = (Color){
        (uchar)(0 * 255.0),
        (uchar)(0 * 255.0),
        (uchar)(0 * 255.0),
        255
    };
  }
}
