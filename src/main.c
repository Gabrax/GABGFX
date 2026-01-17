#include "gabgfx.h"
#include "raylib.h"

/*const char* map =*/
/*"1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \*/
/*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \*/
/*1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, \*/
/*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \*/
/*1, 0, 2, 0, 4, 4, 0, 0, 0, 0, 1, \*/
/*1, 0, 0, 0, 4, 0, 0, 0, 0, 0, 1, \*/
/*1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, \*/
/*1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, \*/
/*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \*/
/*1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \*/
/*1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,";*/

const char* textures[] = {
  "res/greystone.png",
  "res/wood.png",
  "res/mossy.png",
  "res/purplestone.png",
  "res/redbrick.png",
  "res/colorstone.png",
  "res/bluestone.png",
  "res/eagle.png",
};

const char* sprites[] = {
  "res/barrel.png",
  "res/pillar.png",
  "res/greenlight.png",
  "res/demon.png",
  "res/bullet.png",
  "res/enemy1.png",
  "res/enemy2.png",
  "res/enemy3.png",
  "res/enemy4.png",
  "res/shotgun1.png",
  "res/shotgun2.png",
  "res/shotgun3.png",
  "res/shotgun4.png",
  "res/shotgun5.png",
  "res/shotgun6.png",
  "res/shotgun7.png",
  "res/shotgun8.png",
};

SpriteData sprites_data[] = {
  {3.5, 7.5, 0, 0, 0, 0,0, 0, 0, 8},
  {3.5, 5.5, 0, 0, 0, 0,0, 0, 0, 11},
  {3.5, 3.5, 0, 0, 0, 0,0, 0, 0, 9},
  {7.5, 3.5, 0, 0, 0, 0,0, 0, 0, 8},
  {5.5, 5.5, 0, 0, 0, 0,0, 0, 0, 10}
};

#define ARR_SIZE(x) (sizeof x / sizeof x[0])

int main(void)
{
  InitWindow(800, 600, "GABGFX");
  SetTargetFPS(60);
  DisableCursor();

  gfx_init(RAYTRACER);

  /*gfx_load_assets(textures, ARR_SIZE(textures), sprites, ARR_SIZE(sprites), sprites_data, ARR_SIZE(sprites_data));*/

  while (!WindowShouldClose())
  {
    gfx_start_draw();

    if(IsKeyDown(KEY_W)) gfx_move_camera(FORWARD);
    if(IsKeyDown(KEY_S)) gfx_move_camera(BACKWARD);
    if(IsKeyDown(KEY_A)) gfx_move_camera(LEFT);
    if(IsKeyDown(KEY_D)) gfx_move_camera(RIGHT);

    gfx_update_camera();

    gfx_end_draw();
  }

  gfx_close();
}
