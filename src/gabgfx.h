#pragma once

#include <stdint.h>

#include "gab_math.h"
#include "raylib.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"

typedef enum { RASTERIZER, RAYCASTER, RAYTRACER } RenderMode;

typedef enum { FORWARD, BACKWARD, LEFT, RIGHT } Movement;

typedef struct {
    float x, y, vx, vy, dir_x, dir_y;
    int is_projectile, is_ui, is_destroyed, texture;
} SpriteData;

void gfx_init(RenderMode mode);
void gfx_start_draw();
void gfx_end_draw();
void gfx_close();

void gfx_init_camera(float fov, float near_plane, float far_plane);
void gfx_move_camera(Movement direction);
void gfx_update_camera();

void gfx_load_model(const char* filePath,const char* texturePath,f4x4 transform);
void gfx_upload_models_data();
void gfx_print_model_data();

void gfx_load_assets(const char* textures[],size_t textures_count,const char* sprites[],size_t sprites_count,SpriteData sprites_data[],size_t sprites_data_count);
void gfx_draw_map_state();

