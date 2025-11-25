#include "engine.h"
/*#include "raylib.h"*/

int main()
{
  engine_init("src/shapes.cl",800,600);
  engine_background_color((cl_float4){0.0,0.0,0.0,1.0});

  engine_init_camera(800,600,90.0f,0.01f,1000.0f);

  engine_load_model("res/rayman_2_mdl.obj","res/Rayman.png",MatTransform((f3){1.0f, 0.0f, 3.0f},(f3){0.0f, 180.0f, 0.0f},(f3){0.1f, 0.1f, 0.1f}));

  engine_upload_models_data();

  while (engine_is_running())
  {
    engine_start_drawing();

    /*if (IsKeyDown(KEY_W)) engine_process_camera_keys(FORWARD);*/
    /*if (IsKeyDown(KEY_S)) engine_process_camera_keys(BACKWARD);*/
    /*if (IsKeyDown(KEY_A)) engine_process_camera_keys(LEFT);*/
    /*if (IsKeyDown(KEY_D)) engine_process_camera_keys(RIGHT);*/

    /*engine_update_camera(GetMouseX(), GetMouseY(), true);*/
    engine_send_camera_matrix();

    engine_end_drawing();
    /*printf("%d\n",GetFPS());*/
  }

  engine_free_all_models();
  engine_close();

  return 0;
}

