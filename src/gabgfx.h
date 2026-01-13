#pragma once

#include <stdint.h>

#include "gab_math.h"
#include "raylib.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"

typedef enum { RASTERIZER, RAYCASTER, RAYTRACER } RenderMode;

typedef enum { FORWARD, BACKWARD, LEFT, RIGHT } Movement;

void gfx_init(RenderMode mode, uint32_t window_width, uint32_t window_height);
void gfx_set_background_color(Color color);
void gfx_clear_background();
void gfx_draw();
void gfx_close();

void gfx_init_camera(float fov, float near_plane, float far_plane);
void gfx_move_camera(Movement direction);
void gfx_update_camera();

void gfx_load_model(const char* filePath,const char* texturePath,f4x4 transform);
void gfx_upload_models_data();
void gfx_print_model_data();
void gfx_free_all_models();
