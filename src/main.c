#include "GLFW/glfw3.h"
#include "engine.h"

int main()
{
  engine_init("src/shapes.cl",800,600);
  engine_background_color((cl_float4){0.0f, 0.0f, 0.0f, 1.0f});

  engine_init_camera(800,600,90.0f,0.01f,1000.0f);

  engine_load_model("res/rayman_2_mdl.obj","res/Rayman.png",MatTransform((f3){1.0f, 0.0f, 3.0f},(f3){0.0f, 180.0f, 0.0f},(f3){0.1f, 0.1f, 0.1f}));

  engine_upload_models_data();

  while (engine_is_running())
  {
    engine_start_drawing();

    if (engine_key_down(GLFW_KEY_W)) engine_process_camera_keys(FORWARD);
    if (engine_key_down(GLFW_KEY_S)) engine_process_camera_keys(BACKWARD);
    if (engine_key_down(GLFW_KEY_A)) engine_process_camera_keys(LEFT);
    if (engine_key_down(GLFW_KEY_D)) engine_process_camera_keys(RIGHT);

    engine_update_camera();

    engine_end_drawing();
  }

  engine_free_all_models();
  engine_close();

  return 0;
}

