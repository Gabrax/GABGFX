#pragma once

#include "raylib.h"
#include "../gab_math.h"

typedef enum {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
} Movement;

void raycaster_init(int width, int height);
void raycaster_draw();
void raycaster_close();

void raycaster_update_player();

void raycaster_load_map(const char* sprites[],size_t sprites_count,const char* sprites_data[],size_t sprites_data_count);
void raycaster_draw_map_state();
