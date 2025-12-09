#include "raycaster.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "stb_ds.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "CL/cl.h"
#include "CL/cl_platform.h"

typedef struct Sprite {
  double x;
  double y;
  double vx; // Velocity in X direction
  double vy; // Velocity in Y direction
  double dir_x; // Velocity in X direction
  double dir_y; // Velocity in Y direction
  double is_projectile;
  double is_ui;
  double is_destroyed;
  float texture;
} Sprite;

typedef struct Map {
  size_t map_data_size;
  uint8_t* map_data;
  Sprite* sprites;
} GameData;

static GameData s_map;
static int s_map_dim_size;

static cl_platform_id s_platform;
static cl_device_id s_device;
static cl_program s_program;
static cl_context s_context;
static cl_command_queue s_queue;
static cl_int s_err;

static cl_kernel s_clearKernel;
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

void raycaster_init(int width, int height)
{
  InitWindow(width, height, "Raycaster");
  SetTargetFPS(60);
  DisableCursor();

  clGetPlatformIDs(1, &s_platform, NULL);
  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  s_context = clCreateContext(NULL, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueue(s_context, s_device, 0, NULL);
  
  const char* kernelSource = raycaster_load_kernel("src/rasterizer/rasterizer.cl"); 
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

  clSetKernelArg(s_fragmentKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_fragmentKernel, 2, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_fragmentKernel, 3, sizeof(int), &s_screenResolution[1]);
  clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_depthBuffer);

  Image img = GenImageColor(s_screenResolution[0], s_screenResolution[1], s_backgroundColor);
  s_outputTexture = LoadTextureFromImage(img);
  free(img.data); // pixel buffer is managed by OpenCL

  s_pixelBuffer = (Color*)malloc(s_screenResolution[0] * s_screenResolution[1] * sizeof(Color));
}

void raycaster_close()
{
  free(s_pixelBuffer);

  UnloadTexture(s_outputTexture);
  CloseWindow();

  clReleaseDevice(s_device);
  clReleaseProgram(s_program);
  clReleaseCommandQueue(s_queue);
  clReleaseContext(s_context);

  clReleaseKernel(s_clearKernel);
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

void raycaster_load_map(const char* map_data, const char* sprites_data)
{ 
  size_t len = strlen(map_data);
  memset(&s_map, 0, sizeof(s_map));

  for(size_t i=0;i<len;++i)
  {
    if(isdigit(map_data[i])) arrpush(s_map.map_data,map_data[i]);
  }
  
  s_map.map_data_size = arrlen(s_map.map_data);

  double root = sqrt((double)s_map.map_data_size);
  int dim = (int)root;

  if (dim * dim != s_map.map_data_size)
  {
    fprintf(stderr, "Map has to be square!");
    exit(EXIT_FAILURE);
  }
 
  s_map_dim_size = sqrt(s_map.map_data_size);

  const char* p = sprites_data; 
  while (*p)
  {
    Sprite sprite = {0};

    int parsed = sscanf(
        p,
        "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %f",
        &sprite.x, &sprite.y, &sprite.vx, &sprite.vy,
        &sprite.dir_x, &sprite.dir_y,
        &sprite.is_projectile, &sprite.is_ui,
        &sprite.is_destroyed, &sprite.texture
    );

    if (parsed != 10)
        break;

    arrpush(s_map.sprites, sprite);

    // skip to next entry
    for (int commas = 0; *p && commas < 10; p++)
        if (*p == ',') commas++;
  }
}

static int tile_size = 20;

void raycaster_draw_map_state()
{
  int y_offset = 0;

  for(size_t row=0;row<s_map_dim_size;++row)
  {
    int x_offset = 0;
    for(size_t col=0;col<s_map_dim_size;++col)
    {
      int val = s_map.map_data[row * s_map_dim_size + col];
      const char* symbol;
      switch(val)
      {
        case 0: symbol = " "; break;
        case 1: symbol = "#"; break;
        case 2: symbol = "O"; break;
        case 3: symbol = "X"; break;
        case 4: symbol = "@"; break;
      }

      DrawText(symbol, x_offset, y_offset, 6, RAYWHITE);

      x_offset += tile_size;
    }
    y_offset += tile_size;
  }

  /*int p_x_offset = (int)s_player.pos.x * tile_size;*/
  /*int p_y_offset = (int)s_player.pos.y * tile_size;*/
  /*DrawText("P", p_x_offset, p_y_offset, 6, RED);*/
}
