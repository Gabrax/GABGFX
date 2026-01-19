
typedef struct { uchar r,g,b,a; } Color;

typedef struct { float m[4][4]; } Mat4;

typedef struct Vec3 { float x,y,z; } Vec3;
typedef struct Vec4 { float x,y,z,w; } Vec4;

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
  float Roughness;
  float Metallic;
  Vec3 EmissionColor;
  float EmissionPower;
} CustomMaterial;

typedef struct {
  Vec3 pos;
  float radius;
  CustomMaterial material;
} Sphere;

#define PI 3.14159265359f

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

inline float fresnel_schlick_scalar(float cosTheta, float F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

inline float3 fresnel_schlick_vec3(float cosTheta, float3 F0)
{
    float f = pow(1.0f - cosTheta, 5.0f);
    return F0 + (1.0f - F0) * f;
}

inline float3 lerp(float3 a, float3 b, float t)
{
    return a + t * (b - a);
}

inline void BuildONB(float3 N, float3* T, float3* B)
{
    if(fabs(N.z) < 0.999f)
        *T = normalize(cross((float3)(0,0,1), N));
    else
        *T = normalize(cross((float3)(0,1,0), N));

    *B = cross(N, *T);
}

inline float3 SampleCosineHemisphere(float3 N, uint* seed)
{
    float r1 = RandomFloat(seed);
    float r2 = RandomFloat(seed);

    float phi = 2.0f * PI * r1;
    float r   = sqrt(r2);

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(1.0f - r2);

    float3 T, B;
    BuildONB(N, &T, &B);

    return normalize(T * x + B * y + N * z);
}

inline float GGX_D(float3 N, float3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0f);
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;

    return a2 / (PI * denom * denom);
}

inline float GGX_G1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

inline float GGX_G(float3 N, float3 V, float3 L, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);

    return GGX_G1(NdotV, k) * GGX_G1(NdotL, k);
}

inline float3 SampleGGX(float3 N, float roughness, uint* seed,float3 rayDir)
{
    float r1 = RandomFloat(seed);
    float r2 = RandomFloat(seed);

    float a = roughness * roughness;

    float phi = 2.0f * PI * r1;
    float cosTheta = sqrt((1.0f - r2) / (1.0f + (a * a - 1.0f) * r2));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 Hlocal = (float3)(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );

    float3 T, B;
    BuildONB(N, &T, &B);

    float3 H = normalize(T * Hlocal.x + B * Hlocal.y + N * Hlocal.z);

    // reflect view around half vector
    return normalize(reflect_vec(-rayDir, H));
}

inline float GGX_PDF(float3 N, float3 L, float roughness,float3 rayDir)
{
    float3 V = -rayDir;
    float3 H = normalize(V + L);

    float NdotH = max(dot(N, H), 0.0f);
    float VdotH = max(dot(V, H), 0.0f);

    float D = GGX_D(N, H, roughness);

    return (D * NdotH) / max(4.0f * VdotH, 0.001f);
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
    uint spheres_count,
    uint frameIndex,
    __global float4* accumulationBuffer)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  uint idx = y * width + x;

  uint seed = idx;
  seed *= frameIndex;

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

  for (uint bounce = 0; bounce < 5; ++bounce)
  {
    seed += bounce;
    // find closest sphere
    int closestIndex = -1;
    float hitDistance = 1e30f;

    for (uint i = 0; i < spheres_count; ++i) 
    {
      float3 oc = rayOrigin - (float3)(spheres[i].pos.x, spheres[i].pos.y, spheres[i].pos.z);
      float b = 2.0f * dot(oc, rayDir);
      float c = dot(oc, oc) - spheres[i].radius * spheres[i].radius;
      float disc = b*b - 4.0f*c;
      if (disc < 0.0f) continue;

      float t = (-b - sqrt(disc)) * 0.5f;

      if (t > 0.001f && t < hitDistance)
      {
        hitDistance = t;
        closestIndex = i;
      }
    }

    if (closestIndex < 0)
    {
      color += (float3)(0.6f, 0.7f, 0.9f) * throughput;
      break;
    }

    Sphere s = spheres[closestIndex];

    float3 hitPos = rayOrigin + rayDir * hitDistance;
    float3 normal = normalize(hitPos - (float3)(s.pos.x, s.pos.y, s.pos.z));

    CustomMaterial material = s.material;

    float3 Albedo = (float3){material.Albedo.x,material.Albedo.y,material.Albedo.z};
    float metallic  = clamp(material.Metallic,  0.0f, 1.0f);
    float roughness = clamp(material.Roughness, 0.0f, 1.0f);

    float3 V = -rayDir;
    float cosThetaV = max(dot(normal, V), 0.0f);

    float3 F0 = lerp((float3)(0.04f), Albedo, metallic);
    float3 F = fresnel_schlick_vec3(cosThetaV, F0);

    float3 kd = (1.0f - F) * (1.0f - metallic);
    float3 diffuseBRDF = kd * Albedo * (1.0f / PI);

    if(material.EmissionPower > 0.0f)
    {
        color += throughput * Albedo * material.EmissionPower;
        break;
    }

    float specularChance = clamp(max(F.x, max(F.y, F.z)),0.05f,0.95f);

    float3 newDir;
    float pdf;
    float3 BRDF;

    float rand = RandomFloat(&seed);

    // MIRROR
    if(material.Roughness < 0.001f && metallic > 0.99f)
    {
      newDir = reflect_vec(rayDir, normal);

      throughput *= F;

      rayOrigin = hitPos + normal * 0.0001f;
      rayDir    = normalize(newDir);
      continue;
    }
    // SPECULAR
    if(rand < specularChance)
    {
      newDir = SampleGGX(normal, roughness, &seed,rayDir);

      float cosThetaL = max(dot(normal, newDir), 0.0f);

      float D = GGX_D(normal, newDir, roughness);
      float G = GGX_G(normal, V, newDir, roughness);

      float3 specularBRDF =
          (D * G * F) /
          max(4.0f * cosThetaV * cosThetaL, 0.001f);

      BRDF = specularBRDF;

      pdf = specularChance * GGX_PDF(normal, newDir, roughness,rayDir);
    }
    // DIFFUSE
    else
    {
      newDir = SampleCosineHemisphere(normal, &seed);

      float cosThetaL = max(dot(normal, newDir), 0.0f);

      BRDF = diffuseBRDF;

      pdf = (1.0f - specularChance) * (cosThetaL / PI);
    }

    float cosOut = max(dot(normal, newDir), 0.0f);
    throughput *= BRDF * cosOut / max(pdf, 0.001f);

    // NEXT RAY
    rayOrigin = hitPos + normal * 0.0001f;
    rayDir    = normalize(newDir);
  }

  // PATH TRACING
  accumulationBuffer[idx] += (float4){color.xyz,1.0};
  float4 accumulatedColor = accumulationBuffer[idx];
  accumulatedColor /= (float)frameIndex;

  frameBuffer[idx] = (Color){
      (uchar)(clamp(accumulatedColor.x, 0.0f, 1.0f) * 255.0f),
      (uchar)(clamp(accumulatedColor.y, 0.0f, 1.0f) * 255.0f),
      (uchar)(clamp(accumulatedColor.z, 0.0f, 1.0f) * 255.0f),
      255
  };
}
