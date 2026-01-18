#include "gabgfx.h"

#define GABMATH_IMPLEMENTATION
#include "gabmath.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

const char* getErrorString(cl_int error)
{
switch(error){
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
}

#define CL_CHECK_PROGRAM(context, filename, program, device) do { \
    FILE* file = fopen(filename, "rb"); \
    if (!file) { \
        printf("Cannot open kernel file: %s\n", filename); \
        program = NULL; \
        exit(1); \
    } \
    fseek(file, 0, SEEK_END); \
    size_t size = ftell(file); \
    rewind(file); \
    char* src = (char*)malloc(size + 1); \
    fread(src, 1, size, file); \
    src[size] = '\0'; \
    fclose(file); \
    program = clCreateProgramWithSource(context, 1, (const char**)&src, NULL, &s_err); \
    free(src); \
    if (s_err != CL_SUCCESS) { \
        printf("clCreateProgramWithSource failed: %d at %s:%d\n", s_err, __FILE__, __LINE__); \
        printf("OpenCL error :%s\n",getErrorString(s_err)); \
        program = NULL; \
        exit(1); \
    } \
    s_err = clBuildProgram(program, 1, &device, NULL, NULL, NULL); \
    if (s_err != CL_SUCCESS) { \
        size_t log_size; \
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size); \
        char* log = (char*)malloc(log_size); \
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL); \
        printf("OpenCL build error in %s:\n%s\n", filename, log); \
        free(log); \
        clReleaseProgram(program); \
        program = NULL; \
        exit(1); \
    } \
} while(0)

#define CL_CHECK_KERNEL(var, name) do { \
    var = clCreateKernel(s_program, name, &s_err); \
    if (s_err != CL_SUCCESS) { \
        printf("Failed to create kernel '%s': at %s:%d\n", \
               #name, __FILE__, __LINE__); \
        printf("OpenCL error :%s\n",getErrorString(s_err)); \
        exit(1); \
    } \
} while (0)

#define CL_CHECK_BUFFER(buffer, cl_enum, buffer_size, host_ptr) do { \
    buffer = clCreateBuffer(s_context, cl_enum, buffer_size, host_ptr, &s_err); \
    if (s_err != CL_SUCCESS) { \
        printf("Failed to create buffer '%s': at %s:%d\n", \
               #buffer, __FILE__, __LINE__); \
        printf("OpenCL error :%s\n",getErrorString(s_err)); \
        exit(1); \
    } \
} while (0)

#define CL_CHECK_SET_KERNEL_ARG(kernel, arg_index, arg_size, arg) do { \
    s_err = clSetKernelArg(kernel, arg_index, arg_size, &arg); \
    if (s_err != CL_SUCCESS) { \
        printf("Failed to set arg '%s': at %s:%d\n", \
               #arg, __FILE__, __LINE__); \
        printf("OpenCL error :%s\n",getErrorString(s_err)); \
        exit(1); \
    } \
} while (0)

#define CL_CHECK_WRITE_BUFFER(buffer,block,offset,arg_size,arg) do { \
    s_err = clEnqueueWriteBuffer( \
        s_queue, buffer, block, offset, arg_size, arg, \
        0, NULL, NULL \
    ); \
    if (s_err != CL_SUCCESS) { \
        printf("Failed to write buffer '%s': %s:%d\n", \
               #arg, __FILE__, __LINE__); \
        printf("OpenCL error: %s\n", getErrorString(s_err)); \
        exit(1); \
    } \
} while(0)

#define CL_CHECK(func) do { \
  s_err = func; \
  if (s_err != CL_SUCCESS) { \
      printf("Failed at %s:%d\n", __FILE__, __LINE__); \
      printf("OpenCL error :%s\n",getErrorString(s_err)); \
      exit(1); \
  } \
} while(0)

static RenderMode s_mode;

static cl_platform_id s_platform;
static cl_device_id s_device;
static cl_program s_program;
static cl_context s_context;
static cl_command_queue s_queue;
static cl_int s_err;

static cl_kernel s_clearKernel;
static cl_kernel s_vertexKernel;
static cl_kernel s_fragmentKernel;

static cl_kernel s_surfaceKernel;
static cl_kernel s_spritesKernel;

static cl_mem s_frameBuffer;
static cl_mem s_depthBuffer;
static cl_mem s_projectedVertsBuffer;
static cl_mem s_fragPosBuffer;
static cl_mem s_projectionBuffer;
static cl_mem s_inverseProjectionBuffer;
static cl_mem s_viewBuffer;
static cl_mem s_inverseViewBuffer;
static cl_mem s_cameraPosBuffer;
static cl_mem s_trianglesBuffer;
static cl_mem s_pixelsBuffer;
static cl_mem s_modelsBuffer;
static cl_mem s_spheresBuffer;
static cl_mem s_accumulationBuffer;

static cl_mem s_playerBuffer;
static cl_mem s_spritesBuffer;
static cl_mem s_textureBuffer;
static cl_mem s_spritesDataBuffer;
static cl_mem s_mapBuffer;
static cl_mem s_spriteOrderBuffer;
static cl_mem s_spriteDistanceBuffer;

typedef struct {
  Vec3 pos, front, up, right, world_up;
  Mat4 proj, inverse_proj, view, inverse_view;
  float near_plane, far_plane;
  float fov, fov_rad;
  float aspect_ratio;
  float yaw, pitch, speed, sens, lastX, lastY;
  bool firstMouse;
  float deltaTime;
  bool hasMoved;
} CustomCamera;

static CustomCamera s_camera = {0};

typedef struct {
  Vec3 vertex[3]; 
  Vec3 normal[3];
  Vec2 uv[3];
  int modelIdx;
} Triangle;

typedef struct {
  int triangleOffset, triangleCount;
  int vertexOffset, vertexCount;
  int pixelOffset, texWidth, texHeight;
  Mat4 transform;
} CustomModel;

typedef struct { int offset, width, height; } Sprite;

typedef struct { float dist; int index; } SpriteSort;

typedef struct {
  float x, y;
  float dirX, dirY;
  float planeX, planeY;
  float moveSpeed, rotSpeed;
} Player;

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

static Sphere* s_Spheres = NULL;

static uint32_t s_frameIndex = 1;

static Color s_backgroundColor;
static size_t s_screenSize[2];
static uint32_t s_width;
static uint32_t s_height;
static uint32_t s_screenResolution;
static Color* s_pixelBuffer = NULL;
static Texture2D s_outputTexture;

static Triangle* s_allTriangles = NULL;
static Color* s_allTexturePixels = NULL;
static CustomModel* s_Models = NULL;

static size_t s_totalTriangles = 0;
static size_t s_totalVerts = 0;
static size_t s_totalTexturePixels = 0;
static size_t s_triOffset = 0;
static size_t s_pixOffset = 0;

static Player s_Player;

static Sprite* s_Sprites = NULL;
static Color* texture_atlas = NULL;

static size_t s_numSprites;
static int s_spriteOrder[120];
static SpriteData s_spritesData[120]; 

static int s_ui_first_frame = 17;
static int s_ui_last_frame  = 23;
static int s_ui_current_frame = 17;

static int s_ui_anim_playing = 0;
static float s_ui_anim_timer = 0.0f;
static float s_ui_anim_fps   = 6.0f;

static Vec4 zero = { 0.0f, 0.0f, 0.0f, 0.0f };

unsigned char map[11][11] = {
    {1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,5,5,0,0,0,4,4,0,1},
    {1,0,5,0,0,0,0,0,4,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,2,0,0,0,0,0,3,0,1},
    {1,0,2,2,0,0,0,3,3,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1}
};

static inline int sprite_cmp(const void* a, const void* b)
{
    float d = ((SpriteSort*)b)->dist - ((SpriteSort*)a)->dist;
    return (d > 0) - (d < 0); // malejąco
}

static inline void sortSprites(
    Player* p,
    SpriteData* sprites,
    int num,
    int* spriteOrder)
{
  SpriteSort tmp[120];

  for (int i = 0; i < num; i++) {
      float dx = p->x - sprites[i].x;
      float dy = p->y - sprites[i].y;
      tmp[i].dist = dx*dx + dy*dy;
      tmp[i].index = i;
  }

  qsort(tmp, num, sizeof(SpriteSort), sprite_cmp);

  for (int i = 0; i < num; i++)
      spriteOrder[i] = tmp[i].index;
}

void gfx_init(RenderMode mode)
{
  if(!IsWindowReady())
  {
    printf("Initialize window first! - InitWindow()");
    exit(1);
  }
  
  s_screenSize[0] = GetScreenWidth(); s_screenSize[1] = GetScreenHeight();

  s_mode = mode;

  clGetPlatformIDs(1, &s_platform, NULL);
  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  s_context = clCreateContext(NULL, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueue(s_context, s_device, 0, NULL);
  
  if(s_mode == RASTERIZER)
  {
    CL_CHECK_PROGRAM(s_context, "src/rasterizer.cl", s_program, s_device);

    CL_CHECK_KERNEL(s_clearKernel,"clear_buffers");
    CL_CHECK_KERNEL(s_vertexKernel,"vertex_kernel");
    CL_CHECK_KERNEL(s_fragmentKernel,"fragment_kernel");

    CL_CHECK_BUFFER(s_frameBuffer,CL_MEM_READ_WRITE,sizeof(Color)*s_screenSize[0]*s_screenSize[1],NULL);
    CL_CHECK_BUFFER(s_depthBuffer,CL_MEM_READ_WRITE,sizeof(uint32_t)*s_screenSize[0]*s_screenSize[1],NULL);

    CL_CHECK_SET_KERNEL_ARG(s_clearKernel, 0, sizeof(cl_mem), s_frameBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_clearKernel, 1, sizeof(cl_mem), s_depthBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_clearKernel, 2, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_clearKernel, 3, sizeof(int), s_screenSize[1]);
    CL_CHECK_SET_KERNEL_ARG(s_clearKernel, 4, sizeof(Color), ((Color){0,0,0,255}));

    CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 8, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 9, sizeof(int), s_screenSize[1]);

    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 0, sizeof(cl_mem), s_frameBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 2, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 3, sizeof(int), s_screenSize[1]);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 4, sizeof(cl_mem), s_depthBuffer);
  }
  else if(s_mode == RAYCASTER)
  {
    CL_CHECK_PROGRAM(s_context, "src/raycaster.cl", s_program, s_device);

    CL_CHECK_KERNEL(s_surfaceKernel,"surface_kernel");
    CL_CHECK_KERNEL(s_spritesKernel,"sprites_kernel");

    CL_CHECK_BUFFER(s_frameBuffer,CL_MEM_READ_WRITE,sizeof(Color)*s_screenSize[0]*s_screenSize[1],NULL);
    CL_CHECK_BUFFER(s_depthBuffer,CL_MEM_READ_WRITE,sizeof(float)*s_screenSize[0],NULL);
    CL_CHECK_BUFFER(s_playerBuffer,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,sizeof(Player), &s_Player);
    CL_CHECK_BUFFER(s_mapBuffer,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,sizeof(map), &map);

    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 0, sizeof(cl_mem), s_frameBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 1, sizeof(cl_mem), s_depthBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 2, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 3, sizeof(int), s_screenSize[1]);
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 4, sizeof(cl_mem), s_playerBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 5, sizeof(cl_mem), s_mapBuffer);
    int map_size = 11;
    CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 6, sizeof(int), map_size);

    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 0, sizeof(cl_mem), s_frameBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 1, sizeof(cl_mem), s_depthBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 2, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 3, sizeof(int), s_screenSize[1]);
    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 4, sizeof(cl_mem), s_playerBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 10, sizeof(int), s_ui_first_frame);

    s_Player = (Player){5.5f,5.5f,-1.0f,0.0f,0.0f,0.66f,0.05f,0.03f};
  }
  else if(s_mode == RAYTRACER)
  {
    CL_CHECK_PROGRAM(s_context, "src/raytracer.cl", s_program, s_device);

    /*CL_CHECK_KERNEL(s_clearKernel, "clear_buffers");*/
    /*CL_CHECK_KERNEL(s_vertexKernel, "vertex_kernel");*/
    CL_CHECK_KERNEL(s_fragmentKernel, "fragment_kernel");

    CL_CHECK_BUFFER(s_frameBuffer, CL_MEM_WRITE_ONLY, sizeof(Color) * s_screenSize[0] * s_screenSize[1], NULL);
    CL_CHECK_BUFFER(s_depthBuffer, CL_MEM_READ_WRITE, sizeof(cl_uint) * s_screenSize[0] * s_screenSize[1], NULL);
    CL_CHECK_BUFFER(s_accumulationBuffer, CL_MEM_READ_WRITE, sizeof(Vec4) * s_screenSize[0] * s_screenSize[1], NULL);

    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 0, sizeof(cl_mem), s_frameBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 1, sizeof(cl_mem), s_depthBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 2, sizeof(int), s_screenSize[0]);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 3, sizeof(int), s_screenSize[1]);

    Sphere sphere1 = {
        .pos = (Vec3){0.0f, -0.5f, -2.0f},
        .radius = 0.5f,
        .material = {
            .Albedo = (Vec3){0.9f,0.9f,0.9f},
            .Roughness = 0.0f,
            .Metallic = 1.0f,
            .EmissionColor = sphere1.material.Albedo,
            .EmissionPower = 0.0f
        }
    };
    arrpush(s_Spheres, sphere1);

    Sphere sphere2 = {
        .pos = (Vec3){-1.0f,-0.5f,-2.0f},
        .radius = 0.5f,
        .material = {
            .Albedo = (Vec3){1.0f,1.0f,0.0f},
            .Roughness = 0.1f,
            .Metallic = 0.0f,
            .EmissionColor = sphere2.material.Albedo,
            .EmissionPower = 5.0f
        }
    };
    arrpush(s_Spheres, sphere2);

    Sphere sphere3 = {
        .pos = (Vec3){0.0f,-101.0f,-2.0f},
        .radius = 100.0f,
        .material = {
            .Albedo = (Vec3){0.0f,1.0f,1.0f},
            .Roughness = 0.0f,
            .Metallic = 0.0f,
            .EmissionColor = sphere3.material.Albedo,
            .EmissionPower = 0.0f
        }
    };
    arrpush(s_Spheres, sphere3);

    CL_CHECK_BUFFER(s_spheresBuffer,CL_MEM_READ_ONLY,sizeof(Sphere) * arrlen(s_Spheres),NULL);
    CL_CHECK_WRITE_BUFFER(s_spheresBuffer, CL_TRUE, 0, sizeof(Sphere) * arrlen(s_Spheres), s_Spheres);

    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 9, sizeof(cl_mem), s_spheresBuffer);
    uint32_t size = arrlen(s_Spheres);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 10, sizeof(uint32_t), size);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 12, sizeof(cl_mem), s_accumulationBuffer);
  }

  if(s_mode == RASTERIZER || s_mode == RAYTRACER)
  {
    float near_plane = 0.001f;
    float far_plane = 1000.0f;
    float fov = 90.0f;
    float width = s_screenSize[0];
    float height = s_screenSize[1];

    s_camera.pos = (Vec3){0.0f, 0.0f, 0.0f};
    s_camera.world_up = (Vec3){0.0f, 1.0f, 0.0f};
    s_camera.front = (Vec3){0.0f, 0.0f, 1.0f};
    s_camera.aspect_ratio = (float)width / (float)height;
    s_camera.near_plane = near_plane;
    s_camera.far_plane = far_plane;
    s_camera.fov = fov;
    s_camera.fov_rad = DegToRad(s_camera.fov);  
    s_camera.proj = MatPerspective(s_camera.fov_rad, s_camera.aspect_ratio, s_camera.near_plane, s_camera.far_plane);
    s_camera.inverse_proj = MatInverse(&s_camera.proj);
    s_camera.view = MatLookAt(s_camera.pos, Vec3Add(s_camera.pos, s_camera.front), s_camera.world_up);
    s_camera.yaw = 90.0f;
    s_camera.pitch = 0.0f;
    s_camera.speed = 2.0f;
    s_camera.sens = 0.1f;
    s_camera.lastX = width / 2.0f;
    s_camera.lastY = height / 2.0f;
    s_camera.firstMouse = true;
    s_camera.deltaTime = 1.0/60.0f;

    CL_CHECK_BUFFER(s_projectionBuffer, CL_MEM_READ_ONLY, sizeof(Mat4), NULL);
    CL_CHECK_BUFFER(s_inverseProjectionBuffer, CL_MEM_READ_ONLY, sizeof(Mat4), NULL);
    CL_CHECK_BUFFER(s_viewBuffer, CL_MEM_READ_ONLY, sizeof(Mat4), NULL);
    CL_CHECK_BUFFER(s_inverseViewBuffer, CL_MEM_READ_ONLY, sizeof(Mat4), NULL);
    CL_CHECK_BUFFER(s_cameraPosBuffer, CL_MEM_READ_ONLY, sizeof(Vec3), NULL);

    /*CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 5, sizeof(cl_mem), s_projectionBuffer); */
    /*CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 6, sizeof(cl_mem), s_viewBuffer); */
    /*CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 7, sizeof(cl_mem), s_cameraPosBuffer);*/

    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 4, sizeof(cl_mem), s_projectionBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 5, sizeof(cl_mem), s_inverseProjectionBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 6, sizeof(cl_mem), s_viewBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 7, sizeof(cl_mem), s_inverseViewBuffer);
    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 8, sizeof(cl_mem), s_cameraPosBuffer);

    CL_CHECK_WRITE_BUFFER(s_projectionBuffer, CL_FALSE, 0, sizeof(Mat4), &s_camera.proj);
    CL_CHECK_WRITE_BUFFER(s_inverseProjectionBuffer, CL_FALSE, 0, sizeof(Mat4), &s_camera.inverse_proj);
  }

  Image img = GenImageColor(s_screenSize[0], s_screenSize[1], s_backgroundColor);
  s_outputTexture = LoadTextureFromImage(img);
  UnloadImage(img);

  s_pixelBuffer = (Color*)malloc(sizeof(Color)*s_screenSize[0]*s_screenSize[1]);
}

void gfx_start_draw(void)
{
  if(s_mode == RASTERIZER)
  {
    clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenSize, NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_vertexKernel, 1, NULL, &s_totalVerts, NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 2, NULL, s_screenSize, NULL, 0, NULL, NULL);
  }
  else if(s_mode == RAYCASTER)
  {
    sortSprites(&s_Player, s_spritesData, s_numSprites, s_spriteOrder);
  
    CL_CHECK_WRITE_BUFFER(s_spriteOrderBuffer, CL_FALSE, 0, s_numSprites * sizeof(int), s_spriteOrder);
    clEnqueueNDRangeKernel(s_queue, s_surfaceKernel, 1, NULL, &s_screenSize[0], NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_spritesKernel, 1, NULL, &s_screenSize[0], NULL, 0, NULL, NULL);
  }
  else if(s_mode == RAYTRACER)
  {
    if(s_camera.hasMoved)
    {
      s_frameIndex = 1;
      clEnqueueFillBuffer(s_queue, s_accumulationBuffer, &zero, sizeof(Vec4),0,sizeof(Vec4) * s_screenSize[0] * s_screenSize[1],0, NULL, NULL);
    }
    else s_frameIndex++;

    CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 11, sizeof(uint32_t), s_frameIndex);

    clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 2, NULL, s_screenSize, NULL, 0, NULL, NULL);
  }

  CL_CHECK(clEnqueueReadBuffer(s_queue, s_frameBuffer, CL_FALSE, 0, sizeof(Color)*s_screenSize[0]*s_screenSize[1], s_pixelBuffer, 0, NULL, NULL));
  clFinish(s_queue);

  UpdateTexture(s_outputTexture, s_pixelBuffer);
  BeginDrawing();
  DrawTexture(s_outputTexture, 0, 0, WHITE);
}

void gfx_end_draw(void)
{
  EndDrawing();
}

void gfx_close(void)
{
  free(s_pixelBuffer);

  clReleaseDevice(s_device);
  clReleaseProgram(s_program);
  clReleaseCommandQueue(s_queue);
  clReleaseContext(s_context);

  clReleaseKernel(s_clearKernel);
  clReleaseKernel(s_vertexKernel);
  clReleaseKernel(s_fragmentKernel);
  clReleaseKernel(s_surfaceKernel);

  clReleaseMemObject(s_frameBuffer);
  clReleaseMemObject(s_depthBuffer);
  clReleaseMemObject(s_projectedVertsBuffer);
  clReleaseMemObject(s_projectionBuffer);
  clReleaseMemObject(s_viewBuffer);
  clReleaseMemObject(s_cameraPosBuffer);
  clReleaseMemObject(s_trianglesBuffer);
  clReleaseMemObject(s_pixelsBuffer);
  clReleaseMemObject(s_modelsBuffer);

  clReleaseMemObject(s_playerBuffer);
  clReleaseMemObject(s_mapBuffer);
  clReleaseMemObject(s_spritesBuffer);
  clReleaseMemObject(s_textureBuffer);
  clReleaseMemObject(s_spritesDataBuffer);

  arrfree(s_allTriangles);
  arrfree(s_allTexturePixels);
  arrfree(s_Models);
  s_triOffset = 0;
  s_pixOffset = 0;
  s_totalTriangles = 0;
  s_totalTexturePixels = 0;

  UnloadTexture(s_outputTexture);
  CloseWindow();
}

void gfx_load_model(const char* filePath, const char* texturePath, Mat4 transform)
{
  const struct aiScene* scene = aiImportFile(
      filePath,
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_GenSmoothNormals |
      aiProcess_ImproveCacheLocality |
      aiProcess_OptimizeMeshes
  );

  if (!scene || scene->mNumMeshes == 0) {
      fprintf(stderr, "Failed to load model: %s\n", filePath);
      aiReleaseImport(scene);
      return;
  }

  Triangle* triangles = NULL;
  size_t numTriangles = 0;
  size_t numVertices = 0;

  int modelIndex = arrlen(s_Models);

  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
      const struct aiMesh* mesh = scene->mMeshes[m];

      for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
          const struct aiFace* face = &mesh->mFaces[f];
          if (face->mNumIndices != 3) continue; // skip non-triangles

          Triangle tri = {0};
          for (int i = 0; i < 3; i++) {
              unsigned int idx = face->mIndices[i];

              if (mesh->mVertices) {
                  tri.vertex[i].x = mesh->mVertices[idx].x;
                  tri.vertex[i].y = mesh->mVertices[idx].y;
                  tri.vertex[i].z = mesh->mVertices[idx].z;
              }
              if (mesh->mNormals) {
                  tri.normal[i].x = mesh->mNormals[idx].x;
                  tri.normal[i].y = mesh->mNormals[idx].y;
                  tri.normal[i].z = mesh->mNormals[idx].z;
              }
              if (mesh->mTextureCoords[0]) {
                  tri.uv[i].x = mesh->mTextureCoords[0][idx].x;
                  tri.uv[i].y = mesh->mTextureCoords[0][idx].y;
              }
          }
          
          tri.modelIdx = modelIndex;

          arrpush(triangles, tri);
          numTriangles++;
          numVertices += 3;
      }
  }

  aiReleaseImport(scene);

  int texWidth = 0, texHeight = 0;
  Color* pixels = NULL;

  if (texturePath) {
      Image img = LoadImage(texturePath);
      texWidth = img.width;
      texHeight = img.height;

      if (texWidth > 0 && texHeight > 0) {
          pixels = (Color*)malloc(texWidth * texHeight * sizeof(Color));
          memcpy(pixels, img.data, texWidth * texHeight * sizeof(Color));
      }

      UnloadImage(img);
  }

  for (size_t t = 0; t < numTriangles; t++)
      arrpush(s_allTriangles, triangles[t]);

  if (pixels) {
      size_t numPixels = texWidth * texHeight;
      for (size_t p = 0; p < numPixels; p++)
          arrpush(s_allTexturePixels, pixels[p]);
      free(pixels);
  }

  CustomModel m;
  m.triangleOffset = s_triOffset;
  m.triangleCount  = (int)numTriangles;
  m.vertexOffset   = 0;
  m.vertexCount    = (int)numVertices;
  m.pixelOffset    = s_pixOffset;
  m.texWidth       = texWidth;
  m.texHeight      = texHeight;
  m.transform      = transform;
  arrpush(s_Models, m);

  s_triOffset += numTriangles;
  s_pixOffset += texWidth * texHeight;
  s_totalTriangles += numTriangles;
  s_totalTexturePixels += texWidth * texHeight;

  arrfree(triangles);
}

void gfx_upload_models_data(void)
{  
  int numModels = arrlen(s_Models);
  s_totalVerts = s_totalTriangles * 3;

  s_trianglesBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_allTriangles) * sizeof(Triangle), s_allTriangles, &s_err);

  s_pixelsBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_allTexturePixels) * sizeof(Color), s_allTexturePixels, &s_err);

  s_modelsBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_Models) * sizeof(CustomModel), s_Models, &s_err);

  s_projectedVertsBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE,
                                          sizeof(Vec4) * s_totalVerts, NULL, NULL);

  CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 4, sizeof(cl_mem), s_projectedVertsBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 1, sizeof(cl_mem), s_projectedVertsBuffer);

  CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 0, sizeof(cl_mem), s_trianglesBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 1, sizeof(cl_mem), s_modelsBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 2, sizeof(int), numModels);
  CL_CHECK_SET_KERNEL_ARG(s_vertexKernel, 3, sizeof(int), s_totalVerts);

  CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 6, sizeof(cl_mem), s_trianglesBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 7, sizeof(cl_mem), s_modelsBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 8, sizeof(int), numModels);
  CL_CHECK_SET_KERNEL_ARG(s_fragmentKernel, 9, sizeof(cl_mem), s_pixelsBuffer);
}

void gfx_print_model_data(void)
{
  for (size_t m = 0; m < arrlen(s_Models); m++)
  {
    CustomModel* model = &s_Models[m];
    printf("Model %zu:\n", m);
    printf("  Triangles: %d\n", model->triangleCount);
    printf("  Texture size: %dx%d\n", model->texWidth, model->texHeight);
    printf("  Transform matrix:\n");
    MatPrint(&model->transform); 

    for (int t = 0; t < model->triangleCount; t++)
    {
      const Triangle* tri = &s_allTriangles[model->triangleOffset + t];
      printf("  Triangle %d:\n", t);
      for (int v = 0; v < 3; v++)
      {
        printf("    Vertex %d:\n", v);
        printf("      Position: (%f, %f, %f)\n",
               tri->vertex[v].x,
               tri->vertex[v].y,
               tri->vertex[v].z);
        printf("      UV:       (%f, %f)\n",
               tri->uv[v].x,
               tri->uv[v].y);
        printf("      Normal:   (%f, %f, %f)\n",
               tri->normal[v].x,
               tri->normal[v].y,
               tri->normal[v].z);
      }
    }
    printf("\n");
  }
}

void gfx_move_camera(Movement direction)
{
  float velocity = s_camera.speed * s_camera.deltaTime;

  if (direction == FORWARD)
  {
    if(s_mode == RASTERIZER || s_mode == RAYTRACER)
    {
      s_camera.hasMoved = true;
      s_camera.pos = Vec3Add(s_camera.pos, Vec3MulS(s_camera.front, -velocity));
    }
    else if(s_mode == RAYCASTER)
    {
      float nx = s_Player.x + s_Player.dirX * s_Player.moveSpeed;
      float ny = s_Player.y + s_Player.dirY * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
    }
  }
  if (direction == BACKWARD)
  {
    if(s_mode == RASTERIZER || s_mode == RAYTRACER)
    {
      s_camera.hasMoved = true;
      s_camera.pos = Vec3Add(s_camera.pos, Vec3MulS(s_camera.front, velocity));
    }
    else if(s_mode == RAYCASTER)
    {
      float nx = s_Player.x - s_Player.dirX * s_Player.moveSpeed;
      float ny = s_Player.y - s_Player.dirY * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
    }
  }
  if (direction == LEFT)
  {
    if(s_mode == RASTERIZER || s_mode == RAYTRACER)
    {
      s_camera.hasMoved = true;
      s_camera.pos = Vec3Add(s_camera.pos, Vec3MulS(s_camera.right, -velocity));
    }
    else if(s_mode == RAYCASTER)
    {
      float nx = s_Player.x - s_Player.dirY * s_Player.moveSpeed;
      float ny = s_Player.y + s_Player.dirX * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
    }
  }
  if (direction == RIGHT)
  {
    if(s_mode == RASTERIZER || s_mode == RAYTRACER)
    {
      s_camera.hasMoved = true;
      s_camera.pos = Vec3Add(s_camera.pos, Vec3MulS(s_camera.right, velocity));
    }
    else if(s_mode == RAYCASTER)
    {
      float nx = s_Player.x + s_Player.dirY * s_Player.moveSpeed;
      float ny = s_Player.y - s_Player.dirX * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
    }
  }
}
void gfx_update_camera(void)
{
  if(s_mode == RASTERIZER || s_mode == RAYTRACER)
  {
    float mouseX = GetMouseX();
    float mouseY = -GetMouseY();
    bool constrainPitch = true;

    float xoffset, yoffset;
    if (s_camera.firstMouse)
    {
        s_camera.lastX = mouseX;
        s_camera.lastY = mouseY;
        s_camera.firstMouse = false;
    }

    xoffset = (s_camera.lastX - mouseX) * s_camera.sens;
    yoffset = (s_camera.lastY - mouseY) * s_camera.sens; 

    s_camera.lastX = mouseX;
    s_camera.lastY = mouseY;

    if (fabsf(xoffset) > 0.0001f || fabsf(yoffset) > 0.0001f)
    {
      s_camera.hasMoved = true;

      s_camera.yaw   += xoffset;
      s_camera.pitch += yoffset;
    } else s_camera.hasMoved = false;

    if (constrainPitch)
    {
        if (s_camera.pitch > 89.0f)  s_camera.pitch = 89.0f;
        if (s_camera.pitch < -89.0f) s_camera.pitch = -89.0f;
    }

    Vec3 front = {0};
    front.x = cosf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
    front.y = sinf(DegToRad(s_camera.pitch));
    front.z = sinf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
    s_camera.front = Vec3Norm(front);

    s_camera.right = Vec3Norm(Vec3Cross(s_camera.front, s_camera.world_up));
    s_camera.up    = Vec3Norm(Vec3Cross(s_camera.right, s_camera.front));

    s_camera.view = MatLookAt(s_camera.pos, Vec3Add(s_camera.pos, s_camera.front), s_camera.up);

    s_camera.inverse_view = MatInverse(&s_camera.view);

    if(s_camera.hasMoved)
    {
      CL_CHECK_WRITE_BUFFER(s_cameraPosBuffer, CL_FALSE, 0, sizeof(Vec3), &s_camera.pos);
      CL_CHECK_WRITE_BUFFER(s_viewBuffer, CL_FALSE, 0, sizeof(Mat4), &s_camera.view);
      CL_CHECK_WRITE_BUFFER(s_inverseViewBuffer, CL_FALSE, 0, sizeof(Mat4), &s_camera.inverse_view);
    }
  }
  else if(s_mode == RAYCASTER)
  {
    float rot = -GetMouseDelta().x * 0.003;

    float oldDirX = s_Player.dirX;
    s_Player.dirX = s_Player.dirX * cos(rot) - s_Player.dirY * sin(rot);
    s_Player.dirY = oldDirX * sin(rot) + s_Player.dirY * cos(rot);

    float oldPlaneX = s_Player.planeX;
    s_Player.planeX = s_Player.planeX * cos(rot) - s_Player.planeY * sin(rot);
    s_Player.planeY = oldPlaneX * sin(rot) + s_Player.planeY * cos(rot);

    CL_CHECK_WRITE_BUFFER(s_playerBuffer, CL_FALSE, 0, sizeof(Player), &s_Player);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !s_ui_anim_playing)
    {
        s_ui_anim_playing = 1;
        s_ui_anim_timer = 0.0f;
        s_ui_current_frame = s_ui_first_frame;
    }

    if (s_ui_anim_playing)
    {
        s_ui_anim_timer += GetFrameTime();
        float frameTime = 1.0f / s_ui_anim_fps;

        while (s_ui_anim_timer >= frameTime)
        {
            s_ui_anim_timer -= frameTime;
            s_ui_current_frame++;

            if (s_ui_current_frame > s_ui_last_frame)
            {
                s_ui_anim_playing = 0;
                s_ui_current_frame = s_ui_first_frame;
                break;
            }
        }
    }

    CL_CHECK_SET_KERNEL_ARG(s_spritesKernel,10,sizeof(int),s_ui_current_frame);
  }
}

void gfx_load_assets(const char* textures[],size_t textures_count,
                     const char* sprites[],size_t sprites_count,
                     SpriteData sprites_data[],size_t sprites_data_count)
{
  for (size_t i = 0; i < textures_count; ++i)
  {
    Image img = LoadImage(textures[i]);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageFlipVertical(&img);

    int pixel_count = img.width * img.height;
    size_t offset = arrlen(texture_atlas);

    arraddn(texture_atlas, pixel_count);
    memcpy(texture_atlas + offset,
           img.data,
           pixel_count * sizeof(Color));

    Sprite s = {
        .offset = offset,
        .width  = img.width,
        .height = img.height
    };

    arrput(s_Sprites, s);
    UnloadImage(img);
  }
  
  for (size_t i = 0; i < sprites_count; ++i)
  {
    Image img = LoadImage(sprites[i]);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageFlipVertical(&img);

    int pixel_count = img.width * img.height;
    size_t offset = arrlen(texture_atlas);

    arraddn(texture_atlas, pixel_count);
    memcpy(texture_atlas + offset,
           img.data,
           pixel_count * sizeof(Color));

    Sprite s = {
        .offset = offset,
        .width  = img.width,
        .height = img.height
    };

    arrput(s_Sprites, s);
    UnloadImage(img);
  }

  size_t atlas_size = arrlen(texture_atlas);

  s_textureBuffer = clCreateBuffer(
      s_context,
      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      atlas_size * sizeof(Color),
      texture_atlas,
      NULL);
  s_spritesBuffer = clCreateBuffer(
      s_context,
      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      arrlen(s_Sprites) * sizeof(Sprite),
      s_Sprites,
      NULL);
  s_spritesDataBuffer = clCreateBuffer(
      s_context,
      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      sprites_data_count * sizeof(SpriteData),
      sprites_data,
      NULL);
  s_spriteOrderBuffer = clCreateBuffer(
      s_context,
      CL_MEM_READ_ONLY,
      120 * sizeof(int),
      NULL,
      NULL);

  s_numSprites = sprites_count;

  memcpy(s_spritesData, sprites_data, sprites_count * sizeof(SpriteData));

  CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 7, sizeof(cl_mem), s_textureBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_surfaceKernel, 8, sizeof(cl_mem), s_spritesBuffer);

  CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 5, sizeof(cl_mem), s_spritesDataBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 6, sizeof(cl_mem), s_spriteOrderBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 7, sizeof(int), sprites_count);
  CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 8, sizeof(cl_mem), s_textureBuffer);
  CL_CHECK_SET_KERNEL_ARG(s_spritesKernel, 9, sizeof(cl_mem), s_spritesBuffer);
}

static int tile_size = 20;

void gfx_draw_map_state(void)
{
  int y_offset = 0;

  for(size_t row=0;row<11;++row)
  {
    int x_offset = 0;
    for(size_t col=0;col<11;++col)
    {
      int val = map[row][col];

      const char* symbol;
      switch(val)
      {
        case 0: symbol = " "; break;
        case 1: symbol = "#"; break;
        case 2: symbol = "O"; break;
        case 3: symbol = "X"; break;
        case 4: symbol = "@"; break;
        default: symbol = "."; break;
      }

      DrawText(symbol, x_offset, y_offset, 6, RAYWHITE);

      x_offset += tile_size;
    }
    y_offset += tile_size;
  }

  int p_x_offset = (int)s_Player.x * tile_size;
  int p_y_offset = (int)s_Player.y * tile_size;
  DrawText("P", p_x_offset, p_y_offset, 6, RED);
}
