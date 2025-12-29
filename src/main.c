#include "raycaster/raycaster.h"
#include "raylib.h"

const char* map =
"1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
1, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, \
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
1, 0, 2, 0, 4, 4, 0, 0, 0, 0, 1, \
1, 0, 0, 0, 4, 0, 0, 0, 0, 0, 1, \
1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, \
1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, \
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,";

const char* sprites[] = {
  "res/greystone.png",
  "res/wood.png",
  "res/mossy.png",
  "res/purplestone.png",
  "res/redbrick.png",
  "res/colorstone.png",
  "res/bluestone.png",
  "res/eagle.png",
  "res/barrel.png",
  "res/pillar.png",
  "res/greenlight.png",
  "res/demon.png",
  "res/bullet.png",
  "res/enemy1.png",
  "res/enemy2.png",
  "res/enemy3.png",
  "res/enemy4.png",
  "res/shotgun1.png",
  "res/shotgun2.png",
  "res/shotgun3.png",
  "res/shotgun4.png",
  "res/shotgun5.png",
  "res/shotgun6.png",
  "res/shotgun7.png",
  "res/shotgun8.png",
};

const char* sprites_data[] = {
  "6.0, 8.5, 0, 0, 0, 0,0, 0, 0, 10",
  "3.5, 6.5, 0, 0, 0, 0,0, 0, 0, 10",
  "5.5, 5.5, 0, 0, 0, 0,0, 0, 0, 9",
  "7.5, 1.5, 0, 0, 0, 0,0, 0, 0, 8",
  "3.5, 2.5, 0, 0, 0, 0,0, 0, 0, 11"
};

int main()
{
  raycaster_init(800, 600);

  raycaster_load_map(map,sprites,sizeof sprites / sizeof sprites [0],sprites_data,sizeof sprites_data / sizeof sprites_data[0]);

  while(!WindowShouldClose())
  {
    raycaster_clear_background();

    raycaster_draw_map_state();

    raycaster_draw();
  }

  return 0;
}

