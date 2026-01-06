#pragma once

#include "raylib.h"
#include "../gab_math.h"

typedef enum {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
} Movement;

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

void raycaster_init(int width, int height);
void raycaster_draw();
void raycaster_close();

void raycaster_update_player();

void raycaster_load_assets(const char* textures[],size_t textures_count,const char* sprites[],size_t sprites_count,SpriteData sprites_data[],size_t sprites_data_count);
void raycaster_draw_map_state();
