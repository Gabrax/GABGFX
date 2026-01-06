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
    int offset;
    int width;
    int height;
} Sprite;

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

static cl_kernel s_surfaceKernel;
static cl_kernel s_spritesKernel;

static cl_mem s_frameBuffer;
static cl_mem s_depthBuffer;
static cl_mem s_playerBuffer;
static cl_mem s_spritesBuffer;
static cl_mem s_textureBuffer;
static cl_mem s_spritesDataBuffer;
static cl_mem s_mapBuffer;
static cl_mem s_spriteOrderBuffer;
static cl_mem s_spriteDistanceBuffer;

static size_t s_screenResolution[2];
static Color* s_pixelBuffer = NULL;
static Texture2D s_outputTexture;

static Player s_Player;

static Sprite* s_Sprites = NULL;
static Color* texture_atlas = NULL;

unsigned char map[11][11] = {
    {1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,2,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,3,0,1},
    {1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,4,0,0,0,1},
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

int sprite_cmp(const void* a, const void* b)
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
    SpriteSort tmp[120]; // max sprite count

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

  s_surfaceKernel = clCreateKernel(s_program, "surface_kernel", NULL);
  s_spritesKernel = clCreateKernel(s_program, "sprites_kernel", NULL);

  s_frameBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE, sizeof(Color)*width*height, NULL, NULL);
  s_playerBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Player), &s_Player, NULL);
  s_mapBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(map), &map, NULL);
  s_depthBuffer = clCreateBuffer(s_context,CL_MEM_READ_WRITE, sizeof(float)*width,NULL,NULL);

  clSetKernelArg(s_surfaceKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_surfaceKernel, 1, sizeof(cl_mem), &s_depthBuffer);
  clSetKernelArg(s_surfaceKernel, 2, sizeof(int), &width);
  clSetKernelArg(s_surfaceKernel, 3, sizeof(int), &height);
  clSetKernelArg(s_surfaceKernel, 4, sizeof(cl_mem), &s_playerBuffer);
  clSetKernelArg(s_surfaceKernel, 5, sizeof(cl_mem), &s_mapBuffer);
  int map_size = 11;
  clSetKernelArg(s_surfaceKernel, 6, sizeof(int), &map_size);

  clSetKernelArg(s_spritesKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_spritesKernel, 1, sizeof(cl_mem), &s_depthBuffer);
  clSetKernelArg(s_spritesKernel, 2, sizeof(int), &width);
  clSetKernelArg(s_spritesKernel, 3, sizeof(int), &height);
  clSetKernelArg(s_spritesKernel, 4, sizeof(cl_mem), &s_playerBuffer);

  s_pixelBuffer = (Color*)malloc(sizeof(Color)*s_screenResolution[0]*s_screenResolution[1]);
}

static size_t s_numSprites;
static int s_spriteOrder[120];
static SpriteData s_spritesData[120]; 

void raycaster_draw()
{
  prepareSprites(&s_Player, s_spritesData, s_numSprites, s_spriteOrder);

  clEnqueueWriteBuffer(s_queue,s_spriteOrderBuffer,CL_TRUE,0,s_numSprites * sizeof(int),s_spriteOrder,0, NULL, NULL);

  clEnqueueNDRangeKernel(s_queue, s_surfaceKernel, 1, NULL, &s_screenResolution[0], NULL, 0, NULL, NULL);

  clEnqueueNDRangeKernel(s_queue, s_spritesKernel, 1, NULL, &s_screenResolution[0], NULL, 0, NULL, NULL);

  clFinish(s_queue);

  clEnqueueReadBuffer(s_queue, s_frameBuffer, CL_TRUE, 0, sizeof(Color)*s_screenResolution[0]*s_screenResolution[1], s_pixelBuffer, 0, NULL, NULL);

  UpdateTexture(s_outputTexture, s_pixelBuffer);

  BeginDrawing();
  ClearBackground(BLACK);
  DrawTexture(s_outputTexture, 0, 0, WHITE);
  raycaster_draw_map_state();
  EndDrawing();
}

void raycaster_close()
{
  clReleaseMemObject(s_frameBuffer);
  clReleaseMemObject(s_playerBuffer);
  clReleaseMemObject(s_mapBuffer);
  clReleaseMemObject(s_spritesBuffer);
  clReleaseMemObject(s_textureBuffer);
  clReleaseMemObject(s_spritesDataBuffer);

  clReleaseKernel(s_surfaceKernel);
  clReleaseProgram(s_program);
  clReleaseCommandQueue(s_queue);
  clReleaseContext(s_context);

  UnloadTexture(s_outputTexture);
  CloseWindow();
}

void raycaster_load_assets(const char* textures[],size_t textures_count,
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

void raycaster_draw_map_state()
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

