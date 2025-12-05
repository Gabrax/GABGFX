#pragma once

#include "../gab_math.h"
#include <raylib.h>

typedef struct {
    f3 vertex[3]; 
    f3 normal[3];
    f2 uv[3];
    int modelIdx;
} Triangle;

typedef enum {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
} Movement;

void rasterizer_init(int width, int height);
void rasterizer_background_color(Color color);
void rasterizer_clear_background();
void rasterizer_draw();
void rasterizer_close();

void rasterizer_load_model(const char* filePath,const char* texturePath,f4x4 transform);
void rasterizer_upload_models_data();
void rasterizer_print_model_data();
void rasterizer_free_all_models();
void rasterizer_init_camera(float fov, float near_plane, float far_plane);

void rasterizer_process_camera_keys(Movement direction);
void rasterizer_update_camera();



