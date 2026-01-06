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
    float x;
    float y;
    float vx;
    float vy;
    float dir_x;
    float dir_y;
    int is_projectile;
    int is_ui;
    int is_destroyed;
    int texture;
} SpriteData;

void raycaster_init(int width, int height);
void raycaster_draw();
void raycaster_close();

void raycaster_update_player();

void raycaster_load_assets(const char* textures[],size_t textures_count,const char* sprites[],size_t sprites_count,SpriteData sprites_data[],size_t sprites_data_count);
void raycaster_draw_map_state();
