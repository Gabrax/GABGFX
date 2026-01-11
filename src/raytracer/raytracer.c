#include "raytracer.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"
#include "raylib.h"

#define GABMATH_IMPLEMENTATION
#include "../gab_math.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

static cl_platform_id s_platform;
static cl_device_id s_device;
static cl_program s_program;
static cl_context s_context;
static cl_command_queue s_queue;
static cl_int s_err;

static cl_kernel s_clearKernel;
static cl_kernel s_vertexKernel;
static cl_kernel s_fragmentKernel;

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

static Color s_backgroundColor;
static size_t s_screenResolution[2];
static Color* s_pixelBuffer = NULL;
static Texture2D s_outputTexture;

typedef struct {
  f3 Position;
  f3 Front;
  f3 Up;
  f3 Right;
  f3 WorldUp;
  f4x4 proj;
  f4x4 look_at;
  float near_plane;
  float far_plane;
  float fov;
  float fov_rad;
  float aspect_ratio;
  float yaw;
  float pitch;
  float speed;
  float sens;
  float lastX;
  float lastY;
  bool firstMouse;
  float deltaTime;
} CustomCamera;

static CustomCamera s_camera = {0};

typedef struct {
  int triangleOffset;
  int triangleCount;
  int vertexOffset;
  int vertexCount;
  int pixelOffset;
  int texWidth;
  int texHeight;
  f4x4 transform;
} CustomModel;

static Triangle* s_allTriangles = NULL;
static Color* s_allTexturePixels = NULL;
static CustomModel* s_Models = NULL;

static size_t s_totalTriangles = 0;
static size_t s_totalVerts = 0;
static size_t s_totalTexturePixels = 0;
static size_t s_triOffset = 0;
static size_t s_pixOffset = 0;

static const char* raytracer_load_kernel(const char* filename)
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

void raytracer_init(int width, int height)
{
  InitWindow(width, height, "Rasterizer");
  SetTargetFPS(60);
  DisableCursor();

  clGetPlatformIDs(1, &s_platform, NULL);
  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  s_context = clCreateContext(NULL, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueue(s_context, s_device, 0, NULL);
  
  const char* kernelSource = raytracer_load_kernel("src/rasterizer/rasterizer.cl"); 
  s_program = clCreateProgramWithSource(s_context, 1, &kernelSource, NULL, &s_err);
  if (s_err != CL_SUCCESS) { printf("Error creating program: %d\n", s_err); }

  s_err = clBuildProgram(s_program, 1, &s_device, NULL, NULL, NULL);
  if (s_err != CL_SUCCESS) {
      size_t log_size;
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      char* log = (char*)malloc(log_size);
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
      printf("OpenCL build error:\n%s\n", log);
      free(log);
  }

  s_screenResolution[0] = GetScreenWidth();
  s_screenResolution[1] = GetScreenHeight();

  s_clearKernel    = clCreateKernel(s_program, "clear_buffers", NULL);
  s_vertexKernel   = clCreateKernel(s_program, "vertex_kernel", NULL);
  s_fragmentKernel = clCreateKernel(s_program, "fragment_kernel", NULL);

  s_frameBuffer = clCreateBuffer(s_context, CL_MEM_WRITE_ONLY, 
                                      s_screenResolution[0] * s_screenResolution[1]
                                              * sizeof(Color), NULL, NULL);
  s_depthBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE,
                                      sizeof(cl_uint) * s_screenResolution[0]
                                                    * s_screenResolution[1],
                                                    NULL, &s_err);

  clSetKernelArg(s_clearKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_clearKernel, 1, sizeof(cl_mem), &s_depthBuffer);
  clSetKernelArg(s_clearKernel, 2, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_clearKernel, 3, sizeof(int), &s_screenResolution[1]);
  clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);

  clSetKernelArg(s_vertexKernel, 8, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_vertexKernel, 9, sizeof(int), &s_screenResolution[1]);

  clSetKernelArg(s_fragmentKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_fragmentKernel, 2, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_fragmentKernel, 3, sizeof(int), &s_screenResolution[1]);
  clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_depthBuffer);

  Image img = GenImageColor(s_screenResolution[0], s_screenResolution[1], s_backgroundColor);
  s_outputTexture = LoadTextureFromImage(img);
  free(img.data); // pixel buffer is managed by OpenCL

  s_pixelBuffer = (Color*)malloc(s_screenResolution[0] * s_screenResolution[1] * sizeof(Color));
}

void raytracer_background_color(Color color)
{
  clSetKernelArg(s_clearKernel, 4, sizeof(Color), &color);
}

void raytracer_clear_background()
{
  clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);
}

void raytracer_draw()
{
  clEnqueueNDRangeKernel(s_queue, s_vertexKernel, 1, NULL,
                         &s_totalVerts, NULL, 0, NULL, NULL);
  clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 2, NULL,
                         s_screenResolution, NULL, 0, NULL, NULL);
  clEnqueueReadBuffer(s_queue, s_frameBuffer, CL_TRUE, 0,
                      s_screenResolution[0] * s_screenResolution[1] * sizeof(Color),
                      s_pixelBuffer, 0, NULL, NULL);
  clFinish(s_queue);

  UpdateTexture(s_outputTexture, s_pixelBuffer);
  BeginDrawing();
  DrawTexture(s_outputTexture, 0, 0, WHITE);
  EndDrawing();
}

void raytracer_close()
{
  free(s_pixelBuffer);

  UnloadTexture(s_outputTexture);
  CloseWindow();

  clReleaseDevice(s_device);
  clReleaseProgram(s_program);
  clReleaseCommandQueue(s_queue);
  clReleaseContext(s_context);

  clReleaseKernel(s_clearKernel);
  clReleaseKernel(s_vertexKernel);
  clReleaseKernel(s_fragmentKernel);

  clReleaseMemObject(s_frameBuffer);
  clReleaseMemObject(s_depthBuffer);
  clReleaseMemObject(s_projectedVertsBuffer);
  clReleaseMemObject(s_projectionBuffer);
  clReleaseMemObject(s_viewBuffer);
  clReleaseMemObject(s_cameraPosBuffer);
  clReleaseMemObject(s_trianglesBuffer);
  clReleaseMemObject(s_pixelsBuffer);
  clReleaseMemObject(s_modelsBuffer);
}
