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

void raycaster_free_all_textures();

void raycaster_init_camera(float fov, float near_plane, float far_plane);
void raycaster_process_camera_keys(Movement direction);
void raycaster_update_camera();

void raycaster_load_map(const char* map_data, const char* sprites_data);
void raycaster_draw_map_state();
