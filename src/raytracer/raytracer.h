#pragma once

#include "../gab_math.h"
#include "raylib.h"

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

void raytracer_init(int width, int height);
void raytracer_background_color(Color color);
void raytracer_clear_background();
void raytracer_draw();
void raytracer_close();

void raytracer_load_model(const char* filePath,const char* texturePath,f4x4 transform);
void raytracer_upload_models_data();
void raytracer_print_model_data();
void raytracer_free_all_models();

void raytracer_init_camera(float fov, float near_plane, float far_plane);
void raytracer_move_camera(Movement direction);
void raytracer_update_camera();
