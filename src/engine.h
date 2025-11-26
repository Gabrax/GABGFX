#pragma once

#include "gab_math.h"
#include <CL/cl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

void engine_init(const char* kernel,int width, int height);
void engine_background_color(cl_float4 color);
bool engine_is_running();
void engine_start_drawing();
void engine_end_drawing();
void engine_close();

bool engine_key_down(int key);

void engine_load_model(const char* filePath,const char* texturePath,f4x4 transform);
void engine_upload_models_data();
void engine_print_model_data();
void engine_free_all_models();
void engine_init_camera(int width, int height, float fov, float near_plane, float far_plane);

void engine_process_camera_keys(Movement direction);
void engine_update_camera();



