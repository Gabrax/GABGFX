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

typedef struct SpriteData {
  double x;
  double y;
  double vx;
  double vy;
  double dir_x;
  double dir_y;
  double is_projectile;
  double is_ui;
  double is_destroyed;
  float texture;
} SpriteData;

typedef struct {
    float x, y;
    float dirX, dirY;
    float planeX, planeY;
    float moveSpeed;
    float rotSpeed;
} Player;

static cl_platform_id s_platform;
static cl_device_id s_device;
static cl_program s_program;
static cl_context s_context;
static cl_command_queue s_queue;
static cl_int s_err;

static cl_kernel s_fragmentKernel;

static cl_mem s_frameBuffer;
static cl_mem s_depthBuffer;
static cl_mem s_playerBuffer;
static cl_mem s_spritesBuffer;

static cl_mem s_mapBuffer;

static size_t s_screenResolution[2];
static Color* s_pixelBuffer = NULL;
static Texture2D s_outputTexture;

static Player s_Player;

unsigned char map[11][11] = {
    {1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
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

void raycaster_init(int width, int height)
{
  s_screenResolution[0] = width; s_screenResolution[1] = height;

  InitWindow(width, height, "Raycaster");
  SetTargetFPS(60);

  s_Player = (Player){5.5f,5.5f,-1.0f,0.0f,0.0f,0.66f,0.05f,0.03f};

  Image img = GenImageColor(width, height, BLACK);
  s_outputTexture = LoadTextureFromImage(img);
  UnloadImage(img);

  clGetPlatformIDs(1, &s_platform, NULL);

  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  s_context = clCreateContext(NULL, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueueWithProperties(s_context, s_device, 0, NULL);

  const char* kernel_source = raycaster_load_kernel("src/raycaster/raycaster.cl");
  s_program = clCreateProgramWithSource(s_context, 1, &kernel_source, NULL, NULL);
  clBuildProgram(s_program, 1, &s_device, NULL, NULL, NULL);

  s_fragmentKernel = clCreateKernel(s_program, "fragment_kernel", NULL);

  s_frameBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE, sizeof(Color)*width*height, NULL, NULL);
  s_playerBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Player), &s_Player, NULL);
  s_mapBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(map), &map, NULL);

  clSetKernelArg(s_fragmentKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_fragmentKernel, 1, sizeof(int), &width);
  clSetKernelArg(s_fragmentKernel, 2, sizeof(int), &height);
  clSetKernelArg(s_fragmentKernel, 3, sizeof(cl_mem), &s_playerBuffer);
  clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_mapBuffer);
  int map_size = 11;
  clSetKernelArg(s_fragmentKernel, 5, sizeof(int), &map_size);

  s_pixelBuffer = (Color*)malloc(sizeof(Color)*s_screenResolution[0]*s_screenResolution[1]);
}

void raycaster_draw()
{
  clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 1, NULL, &s_screenResolution[0], NULL, 0, NULL, NULL);
  clFinish(s_queue);

  clEnqueueReadBuffer(s_queue, s_frameBuffer, CL_TRUE, 0, sizeof(Color)*s_screenResolution[0]*s_screenResolution[1], s_pixelBuffer, 0, NULL, NULL);

  UpdateTexture(s_outputTexture, s_pixelBuffer);

  BeginDrawing();
  ClearBackground(BLACK);
  DrawTexture(s_outputTexture, 0, 0, WHITE);
  EndDrawing();
}

void raycaster_close()
{
  clReleaseMemObject(s_frameBuffer);
  clReleaseMemObject(s_playerBuffer);
  clReleaseMemObject(s_mapBuffer);
  clReleaseKernel(s_fragmentKernel);
  clReleaseProgram(s_program);
  clReleaseCommandQueue(s_queue);
  clReleaseContext(s_context);

  UnloadTexture(s_outputTexture);
  CloseWindow();
}

void raycaster_load_map(const char* map_data, const char* sprites[],size_t sprites_count,const char* sprites_data[],size_t sprites_data_count)
{
  /*memset(&s_GameData, 0, sizeof(s_GameData));*/
  /**/
  /*for (size_t i = 0; map_data[i]; ++i)*/
  /*    if (isdigit((unsigned char)map_data[i]))*/
  /*        arrpush(s_GameData.map_data, map_data[i] - '0');*/
  /**/
  /*s_GameData.map_data_size = arrlen(s_GameData.map_data);*/
  /**/
  /*int dim = (int)sqrt((double)s_GameData.map_data_size);*/
  /*if (dim * dim != s_GameData.map_data_size)*/
  /*{*/
  /*    fprintf(stderr, "Map has to be square!\n");*/
  /*    exit(EXIT_FAILURE);*/
  /*}*/
  /**/
  /*s_map_dim_size = dim;*/
  /**/
  /*for (size_t i = 0; i < sprites_data_count; ++i)*/
  /*{*/
  /*  const char* p = sprites_data[i];*/
  /**/
  /*  while (*p)*/
  /*  {*/
  /*    SpriteData sprite = {0};*/
  /**/
  /*    int parsed = sscanf(*/
  /*        p,*/
  /*        " %lf , %lf , %lf , %lf , %lf , %lf , %lf , %lf , %lf , %f",*/
  /*        &sprite.x, &sprite.y,*/
  /*        &sprite.vx, &sprite.vy,*/
  /*        &sprite.dir_x, &sprite.dir_y,*/
  /*        &sprite.is_projectile,*/
  /*        &sprite.is_ui,*/
  /*        &sprite.is_destroyed,*/
  /*        &sprite.texture*/
  /*    );*/
  /**/
  /*    if (parsed != 10)*/
  /*        break;*/
  /**/
  /*    arrpush(s_GameData.sprites, sprite);*/
  /**/
  /*    while (*p && *p != ';') p++;*/
  /*    if (*p == ';') p++;*/
  /*  }*/
  /*}*/
  /**/
  /**/
  /*for (size_t i = 0; i < sprites_count; ++i)*/
  /*{*/
  /*    const char* path = sprites[i];*/
  /*    Image img = LoadImage(path);*/
  /**/
  /*    if (img.width > 0 && img.height > 0)*/
  /*    {*/
  /*        // kopiujemy piksele do wspólnego bufora*/
  /*        size_t numPixels = img.width * img.height;*/
  /*        for (size_t p = 0; p < numPixels; ++p)*/
  /*            arrpush(s_allTexturePixels, ((Color*)img.data)[p]);*/
  /**/
  /*        // tworzymy Sprite*/
  /*        Sprite s;*/
  /*        Color* temp = (Color*)malloc(img.width * img.height * sizeof(Color));*/
  /*        memcpy(temp, img.data, img.width * img.height * sizeof(Color));*/
  /**/
  /*        s.pixels = temp;*/
  /*        s.texWidth = img.width;*/
  /*        s.texHeight = img.height;*/
  /*        arrpush(s_Sprites, s);*/
  /**/
  /*        s_pixOffset += numPixels;*/
  /*        s_totalTexturePixels += numPixels;*/
  /*    }*/
  /**/
  /*    UnloadImage(img);*/
  /*}*/
  /**/
  /*s_pixelsBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,*/
  /*      arrlen(s_allTexturePixels) * sizeof(Color), s_allTexturePixels, &s_err);*/
  /**/
  /*s_s_PlayerBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(s_Player), &s_s_Player, &s_err);*/
  /**/
  /*s_game_dataBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,*/
  /*      sizeof(GameData), &s_GameData, &s_err);*/
  /**/
  /*s_spritesBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,*/
  /*      sizeof(Sprite) * arrlen(s_Sprites), &s_Sprites, &s_err);*/
  /**/
  /*clSetKernelArg(s_fragmentKernel, 3, sizeof(cl_mem), &s_s_PlayerBuffer);*/
  /*clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_pixelsBuffer);*/
  /*clSetKernelArg(s_fragmentKernel, 5, sizeof(cl_mem), &s_spritesBuffer);*/
  /*clSetKernelArg(s_fragmentKernel, 6, sizeof(cl_mem), &s_game_dataBuffer);*/
  /*int floor = 6, ceil = 1;*/
  /*clSetKernelArg(s_fragmentKernel, 7, sizeof(int), &floor);*/
  /*clSetKernelArg(s_fragmentKernel, 8, sizeof(int), &ceil);*/
}

static int tile_size = 20;

void raycaster_draw_map_state()
{
  /*int y_offset = 0;*/
  /**/
  /*for(size_t row=0;row<s_map_dim_size;++row)*/
  /*{*/
  /*  int x_offset = 0;*/
  /*  for(size_t col=0;col<s_map_dim_size;++col)*/
  /*  {*/
  /*    int val = s_GameData.map_data[row * s_map_dim_size + col];*/
  // --- Update s_Player in GPU ---
  /*    const char* symbol;*/
  /*    switch(val)*/
  /*    {*/
  /*      case 0: symbol = " "; break;*/
  /*      case 1: symbol = "#"; break;*/
  /*      case 2: symbol = "O"; break;*/
  /*      case 3: symbol = "X"; break;*/
  /*      case 4: symbol = "@"; break;*/
  /*      default: symbol = "."; break;*/
  /*    }*/
  /**/
  /*    DrawText(symbol, x_offset, y_offset, 6, RAYWHITE);*/
  /**/
  /*    x_offset += tile_size;*/
  /*  }*/
  /*  y_offset += tile_size;*/
  /*}*/
  /**/
  /*int p_x_offset = (int)s_s_Player.pos.x * tile_size;*/
  /*int p_y_offset = (int)s_s_Player.pos.y * tile_size;*/
  /*DrawText("P", p_x_offset, p_y_offset, 6, RED);*/
}

void raycaster_update_player()
{
  if (IsKeyDown(KEY_W)){
      float nx = s_Player.x + s_Player.dirX * s_Player.moveSpeed;
      float ny = s_Player.y + s_Player.dirY * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
  }
  if (IsKeyDown(KEY_S)){
      float nx = s_Player.x - s_Player.dirX * s_Player.moveSpeed;
      float ny = s_Player.y - s_Player.dirY * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
  }
  if (IsKeyDown(KEY_A)){
      float nx = s_Player.x - s_Player.dirY * s_Player.moveSpeed;
      float ny = s_Player.y + s_Player.dirX * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
  }
  if (IsKeyDown(KEY_D)){
      float nx = s_Player.x + s_Player.dirY * s_Player.moveSpeed;
      float ny = s_Player.y - s_Player.dirX * s_Player.moveSpeed;
      if(map[(int)ny][(int)nx]==0) { s_Player.x = nx; s_Player.y = ny; }
  }
  if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)){

      float rot = (IsKeyDown(KEY_LEFT) ? s_Player.rotSpeed : -s_Player.rotSpeed);

      float oldDirX = s_Player.dirX;
      s_Player.dirX = s_Player.dirX * cos(rot) - s_Player.dirY * sin(rot);
      s_Player.dirY = oldDirX * sin(rot) + s_Player.dirY * cos(rot);

      float oldPlaneX = s_Player.planeX;
      s_Player.planeX = s_Player.planeX * cos(rot) - s_Player.planeY * sin(rot);
      s_Player.planeY = oldPlaneX * sin(rot) + s_Player.planeY * cos(rot);
  }

  clEnqueueWriteBuffer(s_queue, s_playerBuffer, CL_TRUE, 0, sizeof(Player), &s_Player, 0, NULL, NULL);
}

