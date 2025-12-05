#include "rasterizer/rasterizer.h"
#include "raylib.h"

#include <stdio.h>

int main()
{
  rasterizer_init(800,600);
  rasterizer_background_color((Color){0,0,0,255});

  rasterizer_init_camera(90.0f,0.01f,1000.0f);

  f4x4 transform = MatTransform((f3){1.0f,0.0f,3.0f},(f3){0.0f,180.0f,0.0f},(f3){0.1f,0.1f,0.1f}); 
  rasterizer_load_model("res/rayman_2_mdl.obj","res/Rayman.png",transform);

  rasterizer_upload_models_data();

  while (!WindowShouldClose())
  {
    rasterizer_clear_background();

    if (IsKeyDown(KEY_W)) rasterizer_process_camera_keys(FORWARD);
    if (IsKeyDown(KEY_S)) rasterizer_process_camera_keys(BACKWARD);
    if (IsKeyDown(KEY_A)) rasterizer_process_camera_keys(LEFT);
    if (IsKeyDown(KEY_D)) rasterizer_process_camera_keys(RIGHT);
    rasterizer_update_camera();

    rasterizer_draw();

    printf("%d\n",GetFPS());
  }

  rasterizer_free_all_models();
  rasterizer_close();

  return 0;
}

