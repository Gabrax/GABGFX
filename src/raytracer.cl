
typedef struct { uchar r,g,b,a; } Color;

typedef struct { float m[4][4]; } Mat4;

typedef struct Vec3 { float x,y,z; } Vec3;

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

typedef struct {
  Vec3 Albedo;
} CustomMaterial;

typedef struct {
  Vec3 pos;
  float radius;
  CustomMaterial material;
} Sphere;

inline float4 mul_mat4_vec4(Mat4 m, float4 v)
{
    return (float4)(
        m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z + m.m[0][3]*v.w,
        m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z + m.m[1][3]*v.w,
        m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z + m.m[2][3]*v.w,
        m.m[3][0]*v.x + m.m[3][1]*v.y + m.m[3][2]*v.z + m.m[3][3]*v.w
    );
}

inline uint PCG_Hash(uint input)
{
  uint state = input * 747796405u + 2891336453u;
  uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

inline float RandomFloat(uint* seed)
{
  *seed = PCG_Hash(*seed);
  return (float)(*seed) * (1.0f / 4294967296.0f); // [0,1)
}

inline float3 RandomInUnitSphere(uint* seed)
{
  float3 p;
  do {
      p = (float3)(
          RandomFloat(seed) * 2.0f - 1.0f,
          RandomFloat(seed) * 2.0f - 1.0f,
          RandomFloat(seed) * 2.0f - 1.0f
      );
  } while (dot(p, p) >= 1.0f);

  return p;
}

inline float3 reflect_vec(float3 I, float3 N)
{
    return I - 2.0f * dot(I, N) * N;
}

inline float3 clamp_vec(float3 v, float minVal, float maxVal) {
    return clamp(v, minVal, maxVal);
}

inline float fresnel_schlick(float cosTheta, float F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

__kernel void fragment_kernel(
    __global Color* frameBuffer,
    __global float* depthBuffer,
    int width,
    int height,
    __global Mat4* projection,
    __global Mat4* inverseProjection,
    __global Mat4* view,
    __global Mat4* inverseView,
    __global float3* cameraPos,
    __global Sphere* spheres,
    uint spheres_count)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    uint idx = y * width + x;

    float x_ndc = (2.0f * ((float)x + 0.5f) / width) - 1.0f;
    float y_ndc = 1.0f - (2.0f * ((float)y + 0.5f) / height);

    float4 clip = (float4)(x_ndc, y_ndc, -1.0f, 1.0f);

    float4 viewPos = mul_mat4_vec4(*inverseProjection, clip);
    viewPos /= viewPos.w;

    float3 ray_view = normalize(viewPos.xyz);
    float3 rayDir = normalize(mul_mat4_vec4(*inverseView, (float4)(ray_view, 0.0f)).xyz);
    float3 rayOrigin = *cameraPos;

    float3 color = (float3)(0.0f);
    float3 throughput = (float3)(1.0f);

    float3 lightDir = normalize((float3)(-1.0f, -1.0f, -1.0f));
    float F0 = 0.4f;

    for (uint bounce = 0; bounce < 3; ++bounce) {
        // --- find closest sphere
        int closestIndex = -1;
        float hitDistance = 1e30f;

        for (uint i = 0; i < spheres_count; ++i) {
            float3 oc = rayOrigin - (float3)(spheres[i].pos.x, spheres[i].pos.y, spheres[i].pos.z);
            float b = 2.0f * dot(oc, rayDir);
            float c = dot(oc, oc) - spheres[i].radius * spheres[i].radius;
            float disc = b*b - 4.0f*c;
            if (disc < 0.0f) continue;

            float t = (-b - sqrt(disc)) * 0.5f;
            if (t > 0.001f && t < hitDistance) {
                hitDistance = t;
                closestIndex = i;
            }
        }

        if (closestIndex < 0) {
            color += throughput * (float3)(0.0f, 0.0f, 0.0f);
            break;
        }

        Sphere s = spheres[closestIndex];
        float3 hitPos = rayOrigin + rayDir * hitDistance;
        float3 normal = normalize(hitPos - (float3)(s.pos.x, s.pos.y, s.pos.z));

        float diff = max(dot(normal, -lightDir), 0.0f);
        float3 baseColor = (float3){s.material.Albedo.x, s.material.Albedo.y, s.material.Albedo.z};
        float3 diffuseColor = baseColor * diff;

        float cosTheta = max(dot(-rayDir, normal), 0.0f);
        float reflectivity = fresnel_schlick(cosTheta, F0);

        color += throughput * diffuseColor * (1.0f - reflectivity);

        throughput *= reflectivity;
        rayOrigin = hitPos + normal * 0.0001f;
        rayDir = normalize(reflect_vec(rayDir, normal));
    }

    frameBuffer[idx] = (Color){
        (uchar)(clamp(color.x, 0.0f, 1.0f) * 255.0f),
        (uchar)(clamp(color.y, 0.0f, 1.0f) * 255.0f),
        (uchar)(clamp(color.z, 0.0f, 1.0f) * 255.0f),
        255
    };
}
