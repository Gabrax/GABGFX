typedef struct { uchar r, g, b, a; } Pixel;
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;

typedef struct Mat4 {
    float x0, x1, x2, x3;
    float y0, y1, y2, y3;
    float z0, z1, z2, z3;
    float w0, w1, w2, w3;
} Mat4;

typedef struct {
    Vec3 vertex[3];
    Vec3 normal[3];
    Vec2 uv[3];
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

__kernel void clear_buffers(
    __global Pixel* pixels,
    __global float* depth,
    int width, int height,
    Pixel color)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;
    int idx = y * width + x;
    pixels[idx] = color;
    depth[idx] = FLT_MAX;
}

__kernel void vertex_kernel(
    __global Triangle* tris,
    __global CustomModel* models,
    int numModels,
    int totalVerts,
    __global float4* projVerts,
    __global Mat4* projection,
    __global Mat4* view,
    __global float3* cameraPos,
    int width,
    int height)
{
  int i = get_global_id(0);
  if (i >= totalVerts) return;

  int triIdx = i / 3;
  int vertIdx = i % 3;

  __global const Triangle* tri = &tris[triIdx];
  __global const CustomModel* model = &models[tri->modelIdx];

  float4 vert = (float4)(
      tri->vertex[vertIdx].x,
      tri->vertex[vertIdx].y,
      tri->vertex[vertIdx].z,
      1.0f
  );

  Mat4 transform2 = model->transform;
  float4 v_model;
  v_model.x = vert.x * transform2.x0 + vert.y * transform2.x1 + vert.z * transform2.x2 + vert.w * transform2.x3;
  v_model.y = vert.x * transform2.y0 + vert.y * transform2.y1 + vert.z * transform2.y2 + vert.w * transform2.y3;
  v_model.z = vert.x * transform2.z0 + vert.y * transform2.z1 + vert.z * transform2.z2 + vert.w * transform2.z3;
  v_model.w = vert.x * transform2.w0 + vert.y * transform2.w1 + vert.z * transform2.w2 + vert.w * transform2.w3;

  float4 v_view;
  v_view.x = v_model.x * view->x0 + v_model.y * view->x1 + v_model.z * view->x2 + v_model.w * view->x3;
  v_view.y = v_model.x * view->y0 + v_model.y * view->y1 + v_model.z * view->y2 + v_model.w * view->y3;
  v_view.z = v_model.x * view->z0 + v_model.y * view->z1 + v_model.z * view->z2 + v_model.w * view->z3;
  v_view.w = v_model.x * view->w0 + v_model.y * view->w1 + v_model.z * view->w2 + v_model.w * view->w3;

  float4 v_clip;
  v_clip.x = v_view.x * projection->x0 + v_view.y * projection->x1 + v_view.z * projection->x2 + v_view.w * projection->x3;
  v_clip.y = v_view.x * projection->y0 + v_view.y * projection->y1 + v_view.z * projection->y2 + v_view.w * projection->y3;
  v_clip.z = v_view.x * projection->z0 + v_view.y * projection->z1 + v_view.z * projection->z2 + v_view.w * projection->z3;
  v_clip.w = v_view.x * projection->w0 + v_view.y * projection->w1 + v_view.z * projection->w2 + v_view.w * projection->w3;


  // --- Perspective divide ---
  float ndc_x = v_clip.x / v_clip.w;
  float ndc_y = v_clip.y / v_clip.w;
  float ndc_z = v_clip.z / v_clip.w;

  // --- NDC → screen space ---
  float sx = (ndc_x * 0.5f + 0.5f) * (float)width;
  float sy = (ndc_y * 0.5f + 0.5f) * (float)height;
  float sz = ndc_z * 0.5f + 0.5f;

  projVerts[i] = (float4)(sx, sy, sz, v_clip.w);
}

inline float SignedTriangleArea(float2 a, float2 b, float2 c)
{
    return 0.5f * ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
}

inline Pixel sample_texture(__global Pixel* texture, int texWidth, int texHeight, float2 uv)
{
  uv.x = clamp(uv.x, 0.001f, 0.999f);
  uv.y = clamp(uv.y, 0.001f, 0.999f);

  int u = (int)floor(uv.x * (texWidth - 1) + 0.5f);
  int v = (int)floor((1.0f - uv.y) * (texHeight - 1) + 0.5f);

  return texture[v * texWidth + u];
}

__kernel void fragment_kernel(
    __global Pixel* pixels,
    __global float4* projVerts,
    int width,
    int height,
    __global float* depthBuffer,
    __global float3* cameraPos,
    __global Triangle* tris2,
    __global CustomModel* models,
    int numModels, 
    __global Pixel* textures)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    int idx = y * width + x;
    float2 P = (float2)(x + 0.5f, y + 0.5f); // pixel center

    float3 dirToLight = normalize((float3){5.0f, 5.0f, 0.0f});

    for (int modelidx = 0; modelidx < numModels; modelidx++)
    {
        __global const CustomModel* model = &models[modelidx];

        for (int triIdx = model->triangleOffset;
             triIdx < model->triangleOffset + model->triangleCount;
             triIdx++)
        {
            float4 pv0 = projVerts[triIdx * 3 + 0];
            float4 pv1 = projVerts[triIdx * 3 + 1];
            float4 pv2 = projVerts[triIdx * 3 + 2];

            if (pv0.w >= 0 || pv1.w >= 0 || pv2.w >= 0)
                continue; // discard whole triangle

            float2 v0 = (float2)(pv0.x, pv0.y);
            float2 v1 = (float2)(pv1.x, pv1.y);
            float2 v2 = (float2)(pv2.x, pv2.y);

            float area = (v1.x - v0.x) * (v2.y - v0.y)
                       - (v1.y - v0.y) * (v2.x - v0.x);

            if (area <= 0.0f) continue;  // Cull triangle

            float a = SignedTriangleArea(P, v1, v2) / area;
            float b = SignedTriangleArea(P, v2, v0) / area;
            float g = SignedTriangleArea(P, v0, v1) / area;

            if (a >= 0 && b >= 0 && g >= 0)
            {
                float z0 = pv0.z / pv0.w;
                float z1 = pv1.z / pv1.w;
                float z2 = pv2.z / pv2.w;
                float depth = a*z0 + b*z1 + g*z2;

                if (depth < depthBuffer[idx])
                {
                    __global const Triangle* t = &tris2[triIdx];

                    float2 uv0 = (float2){t->uv[0].x,t->uv[0].y};
                    float2 uv1 = (float2){t->uv[1].x,t->uv[1].y};
                    float2 uv2 = (float2){t->uv[2].x,t->uv[2].y};

                    float2 uv = (uv0 * (a * z0) +
                                 uv1 * (b * z1) +
                                 uv2 * (g * z2)) / depth;

                    float3 norm0 = (float3){t->normal[0].x,t->normal[0].y,t->normal[0].z};
                    float3 norm1 = (float3){t->normal[1].x,t->normal[1].y,t->normal[1].z};
                    float3 norm2 = (float3){t->normal[2].x,t->normal[2].y,t->normal[2].z};
                    float3 norm = normalize((norm0*(a*z0) + norm1*(b*z1) + norm2*(g*z2)) / depth);

                    int texOffset = model->pixelOffset;
                    int tw = model->texWidth;
                    int th = model->texHeight;

                    float3 texColor;
                    if (tw > 0 && th > 0) {
                        Pixel texel = sample_texture(&textures[texOffset], tw, th, uv);
                        texColor = (float3){texel.r, texel.g, texel.b} / 255.0f;
                    } else {
                        texColor = (float3)(0.8f, 0.8f, 0.8f);
                    }
                
                    float light_intensity = fmax(0.1f, dot(norm, dirToLight));
                    float3 finalColor = texColor * light_intensity;

                    pixels[idx] = (Pixel){
                        (uchar)(finalColor.x * 255),
                        (uchar)(finalColor.y * 255),
                        (uchar)(finalColor.z * 255),
                        255
                    };

                    depthBuffer[idx] = depth;
                }
            }
        }
    }
}
