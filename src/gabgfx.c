#include "gabgfx.h"

#define GABMATH_IMPLEMENTATION
#include "gab_math.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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
static cl_mem s_viewBuffer;
static cl_mem s_cameraPosBuffer;
static cl_mem s_trianglesBuffer;
static cl_mem s_pixelsBuffer;
static cl_mem s_modelsBuffer;

static cl_mem s_frameBuffer;
static cl_mem s_depthBuffer;
static cl_mem s_playerBuffer;
static cl_mem s_spritesBuffer;
static cl_mem s_textureBuffer;
static cl_mem s_spritesDataBuffer;
static cl_mem s_mapBuffer;
static cl_mem s_spriteOrderBuffer;
static cl_mem s_spriteDistanceBuffer;

typedef struct {
  f3 Position, Front, Up, Right, WorldUp;
  f4x4 proj, look_at;
  float near_plane, far_plane, fov, fov_rad, aspect_ratio, yaw, pitch, speed, sens, lastX, lastY, deltaTime;
  bool firstMouse;
} CustomCamera;

static CustomCamera s_camera = {0};

typedef struct {
    f3 vertex[3]; 
    f3 normal[3];
    f2 uv[3];
    int modelIdx;
} Triangle;

typedef struct {
  int triangleOffset, triangleCount, vertexOffset, vertexCount, pixelOffset, texWidth, texHeight;
  f4x4 transform;
} CustomModel;

typedef struct {
    int offset, width, height;
} Sprite;

typedef struct {
    float x, y, dirX, dirY, planeX, planeY, moveSpeed, rotSpeed;
} Player;

static Color s_backgroundColor;
static size_t s_screenResolution[2];
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

static size_t s_screenResolution[2];
static Texture2D s_outputTexture;

static Player s_Player;

static Sprite* s_Sprites = NULL;
static Color* texture_atlas = NULL;

static size_t s_numSprites;
static int s_spriteOrder[120];
static SpriteData s_spritesData[120]; 

static int ui_first_frame = 17;
static int ui_last_frame  = 23;
static int ui_current_frame = 17;

static int ui_anim_playing = 0;
static float ui_anim_timer = 0.0f;
static float ui_anim_fps   = 6.0f;

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

static inline const char* raycaster_load_kernel(const char* filename)
{
  FILE* f = fopen(filename, "rb");
  if(!f) { printf("Cannot open kernel file.\n"); return NULL; }
  fseek(f,0,SEEK_END);
  size_t size = ftell(f);
  fseek(f,0,SEEK_SET);
  char* src = (char*)malloc(size+1);
  fread(src,1,size,f);
  src[size] = '\0';
  fclose(f);
  return src;
}

typedef struct {
    float dist;
    int index;
} SpriteSort;

static inline int sprite_cmp(const void* a, const void* b)
{
    float d = ((SpriteSort*)b)->dist - ((SpriteSort*)a)->dist;
    return (d > 0) - (d < 0); // malejąco
}

static inline void prepareSprites(
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

static const char* gfx_load_kernel(const char* filename)
{
  FILE* f = fopen(filename, "rb");
  if(!f) { printf("Cannot open kernel file.\n"); return NULL; }
  fseek(f,0,SEEK_END);
  size_t size = ftell(f);
  fseek(f,0,SEEK_SET);
  char* src = (char*)malloc(size+1);
  fread(src,1,size,f);
  src[size] = '\0';
  fclose(f);
  return src;
}

void gfx_init(RenderMode mode)
{
  if(!IsWindowReady())
  {
    printf("Initialize window first! - InitWindow()");
    exit(1);
  }
  
  s_screenResolution[0] = GetScreenWidth(); s_screenResolution[1] = GetScreenHeight();

  s_mode = mode;

  clGetPlatformIDs(1, &s_platform, NULL);
  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  s_context = clCreateContext(NULL, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueue(s_context, s_device, 0, NULL);
  
  if(s_mode == RASTERIZER)
  {
    const char* kernelSource = gfx_load_kernel("src/rasterizer.cl"); 
    s_program = clCreateProgramWithSource(s_context, 1, &kernelSource, NULL, &s_err);
    if (s_err != CL_SUCCESS) { printf("Error creating program: %d\n", s_err); }

    s_err = clBuildProgram(s_program, 1, &s_device, NULL, NULL, NULL);
    if (s_err != CL_SUCCESS)
    {
      size_t log_size;
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      char* log = (char*)malloc(log_size);
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
      printf("OpenCL build error:\n%s\n", log);
      free(log);
    }

    s_clearKernel    = clCreateKernel(s_program, "clear_buffers", NULL);
    s_vertexKernel   = clCreateKernel(s_program, "vertex_kernel", NULL);
    s_fragmentKernel = clCreateKernel(s_program, "fragment_kernel", NULL);

    s_frameBuffer = clCreateBuffer(s_context, CL_MEM_WRITE_ONLY, s_screenResolution[0] * s_screenResolution[1] * sizeof(Color), NULL, NULL);
    s_depthBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE, sizeof(cl_uint) * s_screenResolution[0] * s_screenResolution[1], NULL, &s_err);

    clSetKernelArg(s_clearKernel, 0, sizeof(cl_mem), &s_frameBuffer);
    clSetKernelArg(s_clearKernel, 1, sizeof(cl_mem), &s_depthBuffer);
    clSetKernelArg(s_clearKernel, 2, sizeof(int), &s_screenResolution[0]);
    clSetKernelArg(s_clearKernel, 3, sizeof(int), &s_screenResolution[1]);
    clSetKernelArg(s_clearKernel, 4, sizeof(Color), &(Color){0,0,0,255});
    clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);

    clSetKernelArg(s_vertexKernel, 8, sizeof(int), &s_screenResolution[0]);
    clSetKernelArg(s_vertexKernel, 9, sizeof(int), &s_screenResolution[1]);

    clSetKernelArg(s_fragmentKernel, 0, sizeof(cl_mem), &s_frameBuffer);
    clSetKernelArg(s_fragmentKernel, 2, sizeof(int), &s_screenResolution[0]);
    clSetKernelArg(s_fragmentKernel, 3, sizeof(int), &s_screenResolution[1]);
    clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_depthBuffer);

  }
  else if(s_mode == RAYCASTER)
  {
    s_Player = (Player){5.5f,5.5f,-1.0f,0.0f,0.0f,0.66f,0.05f,0.03f};

    const char* kernel_source = raycaster_load_kernel("src/raycaster.cl");
    s_program = clCreateProgramWithSource(s_context, 1, &kernel_source, NULL, NULL);

    s_err = clBuildProgram(s_program, 1, &s_device, NULL, NULL, NULL);
    if (s_err != CL_SUCCESS)
    {
      size_t log_size;
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      char* log = (char*)malloc(log_size);
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
      printf("OpenCL build error:\n%s\n", log);
      free(log);
    }

    s_surfaceKernel = clCreateKernel(s_program, "surface_kernel", NULL);
    s_spritesKernel = clCreateKernel(s_program, "sprites_kernel", NULL);

    s_frameBuffer  = clCreateBuffer(s_context, CL_MEM_READ_WRITE, sizeof(Color)*s_screenResolution[0]*s_screenResolution[1], NULL, NULL);
    s_depthBuffer  = clCreateBuffer(s_context,CL_MEM_READ_WRITE, sizeof(float)*s_screenResolution[0],NULL,NULL);
    s_playerBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Player), &s_Player, NULL);
    s_mapBuffer    = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(map), &map, NULL);

    clSetKernelArg(s_surfaceKernel, 0, sizeof(cl_mem), &s_frameBuffer);
    clSetKernelArg(s_surfaceKernel, 1, sizeof(cl_mem), &s_depthBuffer);
    clSetKernelArg(s_surfaceKernel, 2, sizeof(int), &s_screenResolution[0]);
    clSetKernelArg(s_surfaceKernel, 3, sizeof(int), &s_screenResolution[1]);
    clSetKernelArg(s_surfaceKernel, 4, sizeof(cl_mem), &s_playerBuffer);
    clSetKernelArg(s_surfaceKernel, 5, sizeof(cl_mem), &s_mapBuffer);
    int map_size = 11;
    clSetKernelArg(s_surfaceKernel, 6, sizeof(int), &map_size);

    clSetKernelArg(s_spritesKernel, 0, sizeof(cl_mem), &s_frameBuffer);
    clSetKernelArg(s_spritesKernel, 1, sizeof(cl_mem), &s_depthBuffer);
    clSetKernelArg(s_spritesKernel, 2, sizeof(int), &s_screenResolution[0]);
    clSetKernelArg(s_spritesKernel, 3, sizeof(int), &s_screenResolution[1]);
    clSetKernelArg(s_spritesKernel, 4, sizeof(cl_mem), &s_playerBuffer);
    clSetKernelArg(s_spritesKernel, 10, sizeof(int), &ui_first_frame);
  }

  Image img = GenImageColor(s_screenResolution[0], s_screenResolution[1], s_backgroundColor);
  s_outputTexture = LoadTextureFromImage(img);
  UnloadImage(img);

  s_pixelBuffer = (Color*)malloc(sizeof(Color)*s_screenResolution[0]*s_screenResolution[1]);
}

void gfx_start_draw()
{
  if(s_mode == RASTERIZER)
  {
    clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_vertexKernel, 1, NULL, &s_totalVerts, NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);
  }
  else if(s_mode == RAYCASTER)
  {
    prepareSprites(&s_Player, s_spritesData, s_numSprites, s_spriteOrder);

    clEnqueueWriteBuffer(s_queue,s_spriteOrderBuffer,CL_TRUE,0,s_numSprites * sizeof(int),s_spriteOrder,0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_surfaceKernel, 1, NULL, &s_screenResolution[0], NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(s_queue, s_spritesKernel, 1, NULL, &s_screenResolution[0], NULL, 0, NULL, NULL);
  }

  clEnqueueReadBuffer(s_queue, s_frameBuffer, CL_TRUE, 0, sizeof(Color)*s_screenResolution[0]*s_screenResolution[1], s_pixelBuffer, 0, NULL, NULL);
  clFinish(s_queue);

  UpdateTexture(s_outputTexture, s_pixelBuffer);
  BeginDrawing();
  DrawTexture(s_outputTexture, 0, 0, WHITE);
}

void gfx_end_draw()
{
  EndDrawing();
}

void gfx_close()
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

void gfx_load_model(const char* filePath, const char* texturePath, f4x4 transform)
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

void gfx_upload_models_data()
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
                                          sizeof(f4) * s_totalVerts, NULL, NULL);

  clSetKernelArg(s_vertexKernel, 4, sizeof(cl_mem), &s_projectedVertsBuffer);
  clSetKernelArg(s_fragmentKernel, 1, sizeof(cl_mem), &s_projectedVertsBuffer);

  clSetKernelArg(s_vertexKernel, 0, sizeof(cl_mem), &s_trianglesBuffer);
  clSetKernelArg(s_vertexKernel, 1, sizeof(cl_mem), &s_modelsBuffer);
  clSetKernelArg(s_vertexKernel, 2, sizeof(int), &numModels);
  clSetKernelArg(s_vertexKernel, 3, sizeof(int), &s_totalVerts);

  clSetKernelArg(s_fragmentKernel, 6, sizeof(cl_mem), &s_trianglesBuffer);
  clSetKernelArg(s_fragmentKernel, 7, sizeof(cl_mem), &s_modelsBuffer);
  clSetKernelArg(s_fragmentKernel, 8, sizeof(int), &numModels);
  clSetKernelArg(s_fragmentKernel, 9, sizeof(cl_mem), &s_pixelsBuffer);
}

void gfx_print_model_data()
{
    for (size_t m = 0; m < arrlen(s_Models); m++) {
        CustomModel* model = &s_Models[m];
        printf("Model %zu:\n", m);
        printf("  Triangles: %d\n", model->triangleCount);
        printf("  Texture size: %dx%d\n", model->texWidth, model->texHeight);
        printf("  Transform matrix:\n");
        MatPrint(&model->transform); 

        for (int t = 0; t < model->triangleCount; t++) {
            const Triangle* tri = &s_allTriangles[model->triangleOffset + t];
            printf("  Triangle %d:\n", t);
            for (int v = 0; v < 3; v++) {
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

void gfx_init_camera(float fov, float near_plane, float far_plane)
{
  float width = s_screenResolution[0];
  float height = s_screenResolution[1];

  s_camera.Position = (f3){0.0f, 0.0f, 0.0f};
  s_camera.WorldUp = (f3){0.0f, 1.0f, 0.0f};
  s_camera.Front = (f3){0.0f, 0.0f, 1.0f};
  s_camera.aspect_ratio = (float)width / (float)height;
  s_camera.near_plane = near_plane;
  s_camera.far_plane = far_plane;
  s_camera.fov = fov;
  s_camera.fov_rad = DegToRad(s_camera.fov);  
  s_camera.proj = MatPerspective(s_camera.fov_rad, s_camera.aspect_ratio, s_camera.near_plane, s_camera.far_plane);
  s_camera.look_at = MatLookAt(s_camera.Position, f3Add(s_camera.Position, s_camera.Front), s_camera.WorldUp);
  s_camera.yaw = 90.0f;
  s_camera.pitch = 0.0f;
  s_camera.speed = 2.0f;
  s_camera.sens = 0.1f;
  s_camera.lastX = width / 2.0f;
  s_camera.lastY = height / 2.0f;
  s_camera.firstMouse = true;
  s_camera.deltaTime = 1.0/60.0f;

  s_projectionBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f4x4), NULL, &s_err);
  s_viewBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f4x4), NULL, &s_err);
  s_cameraPosBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f3), NULL, &s_err);

  clSetKernelArg(s_vertexKernel, 5, sizeof(cl_mem), &s_projectionBuffer); 
  clSetKernelArg(s_vertexKernel, 6, sizeof(cl_mem), &s_viewBuffer); 
  clSetKernelArg(s_vertexKernel, 7, sizeof(cl_mem), &s_cameraPosBuffer);

  clSetKernelArg(s_fragmentKernel, 5, sizeof(cl_mem), &s_cameraPosBuffer);

  clEnqueueWriteBuffer(s_queue, s_projectionBuffer, CL_TRUE, 0, sizeof(f4x4), &s_camera.proj, 0, NULL, NULL);
}

void gfx_move_camera(Movement direction)
{
  float velocity = s_camera.speed * s_camera.deltaTime;

  if (direction == FORWARD)
  {
    if(s_mode == RASTERIZER)
    {
      s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Front, velocity));
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
    if(s_mode == RASTERIZER)
    {
      s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Front, -velocity));
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
    if(s_mode == RASTERIZER)
    {
      s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Right, velocity));
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
    if(s_mode == RASTERIZER)
    {
      s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Right, -velocity));
    }
    else if(s_mode == RAYCASTER)
    {
      float nx = s_Player.x + s_Player.dirY * s_Player.moveSpeed;
      float ny = s_Player.y - s_Player.dirX * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
    }
  }
}
void gfx_update_camera()
{
  if(s_mode == RASTERIZER)
  {
    float mouseX = GetMouseX();
    float mouseY = GetMouseY();
    bool constrainPitch = true;

    float xoffset, yoffset;
    if (s_camera.firstMouse)
    {
        s_camera.lastX = mouseX;
        s_camera.lastY = mouseY;
        s_camera.firstMouse = false;
    }

    xoffset = s_camera.lastX - mouseX;
    yoffset = s_camera.lastY - mouseY; 

    s_camera.lastX = mouseX;
    s_camera.lastY = mouseY;

    xoffset *= s_camera.sens;
    yoffset *= s_camera.sens;

    s_camera.yaw   += xoffset;
    s_camera.pitch += yoffset;

    if (constrainPitch)
    {
        if (s_camera.pitch > 89.0f)  s_camera.pitch = 89.0f;
        if (s_camera.pitch < -89.0f) s_camera.pitch = -89.0f;
    }

    f3 front;
    front.x = cosf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
    front.y = sinf(DegToRad(s_camera.pitch));
    front.z = sinf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
    s_camera.Front = f3Norm(front);

    s_camera.Right = f3Norm(f3Cross(s_camera.Front, s_camera.WorldUp));
    s_camera.Up    = f3Norm(f3Cross(s_camera.Right, s_camera.Front));

    s_camera.look_at = MatLookAt(s_camera.Position, f3Add(s_camera.Position, s_camera.Front), s_camera.Up);

    clEnqueueWriteBuffer(s_queue, s_cameraPosBuffer, CL_TRUE, 0,
                         sizeof(f3), &s_camera.Position, 0, NULL, NULL);
    clEnqueueWriteBuffer(s_queue, s_viewBuffer, CL_TRUE, 0,
                         sizeof(f4x4), &s_camera.look_at, 0, NULL, NULL);
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

    clEnqueueWriteBuffer(s_queue, s_playerBuffer, CL_TRUE, 0, sizeof(Player), &s_Player, 0, NULL, NULL);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ui_anim_playing)
    {
        ui_anim_playing = 1;
        ui_anim_timer = 0.0f;
        ui_current_frame = ui_first_frame;
    }

    if (ui_anim_playing)
    {
        ui_anim_timer += GetFrameTime();
        float frameTime = 1.0f / ui_anim_fps;

        while (ui_anim_timer >= frameTime)
        {
            ui_anim_timer -= frameTime;
            ui_current_frame++;

            if (ui_current_frame > ui_last_frame)
            {
                ui_anim_playing = 0;
                ui_current_frame = ui_first_frame;
                break;
            }
        }
    }

    clSetKernelArg(s_spritesKernel,10,sizeof(int),&ui_current_frame);
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

  clSetKernelArg(s_surfaceKernel, 7, sizeof(cl_mem), &s_textureBuffer);
  clSetKernelArg(s_surfaceKernel, 8, sizeof(cl_mem), &s_spritesBuffer);

  clSetKernelArg(s_spritesKernel, 5, sizeof(cl_mem), &s_spritesDataBuffer);
  clSetKernelArg(s_spritesKernel, 6, sizeof(cl_mem), &s_spriteOrderBuffer);
  clSetKernelArg(s_spritesKernel, 7, sizeof(int), &sprites_count);
  clSetKernelArg(s_spritesKernel, 8, sizeof(cl_mem), &s_textureBuffer);
  clSetKernelArg(s_spritesKernel, 9, sizeof(cl_mem), &s_spritesBuffer);
}

static int tile_size = 20;

void gfx_draw_map_state()
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
