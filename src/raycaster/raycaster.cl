typedef struct { uchar r, g, b, a; } Pixel;
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;

typedef struct Mat4 {
    float x0, x1, x2, x3;
    float y0, y1, y2, y3;
    float z0, z1, z2, z3;
    float w0, w1, w2, w3;
} Mat4;

typedef struct SpriteData {
  double x;
  double y;
  double vx; // Velocity in X direction
  double vy; // Velocity in Y direction
  double dir_x; // Velocity in X direction
  double dir_y; // Velocity in Y direction
  double is_projectile;
  double is_ui;
  double is_destroyed;
  float texture;
} SpriteData;

typedef struct { float x, y; float dirX, dirY; float planeX, planeY; } Player;
typedef struct { uchar r,g,b,a; } Color;

__kernel void fragment_kernel(
   __global Color* framebuffer,
   int screen_width,
   int screen_height,
   __global Player* player,
   __global uchar* map_data,
   int map_size)
{
   int x = get_global_id(0);
   if(x>=screen_width) return;

   Player p = player[0];

   float cameraX = 2.0f*x/screen_width -1.0f;

   float rayDirX = p.dirX + p.planeX*cameraX;
   float rayDirY = p.dirY + p.planeY*cameraX;

   int mapX = (int)p.x;
   int mapY = (int)p.y;

   float deltaDistX = (rayDirX==0) ? 1e30f : fabs(1.0f/rayDirX);
   float deltaDistY = (rayDirY==0) ? 1e30f : fabs(1.0f/rayDirY);

   int stepX = (rayDirX<0) ? -1 : 1;
   int stepY = (rayDirY<0) ? -1 : 1;

   float sideDistX = (rayDirX<0) ? (p.x-mapX) * deltaDistX : (mapX+1.0f-p.x) * deltaDistX;
   float sideDistY = (rayDirY<0) ? (p.y-mapY) * deltaDistY : (mapY+1.0f-p.y) * deltaDistY;

   int hit=0, side=0;
   while(!hit)
   {
       if(sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side=0; } else { sideDistY += deltaDistY; mapY += stepY; side=1; }
       if(mapX<0 || mapY < 0 || mapX >= map_size || mapY >= map_size) break;
       if(map_data[mapY * map_size + mapX] > 0) hit = 1;
   }

   float perpWallDist = (side==0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);

   perpWallDist = fmax(perpWallDist,0.0001f);
   int lineHeight = (int)(screen_height / perpWallDist);
   int drawStart = max(-lineHeight/2 + screen_height/2, 0);
   int drawEnd = min(lineHeight/2 + screen_height/2, screen_height-1);
   uchar r = (side==0) ? 255 : 180;
   uchar g = 0; uchar b = 0;

   for(int y=0;y<screen_height;y++)
   {
       int idx = y * screen_width + x;
       if(y < drawStart) framebuffer[idx] = (Color){70,70,120,255};
       else if(y > drawEnd) framebuffer[idx] = (Color){40,40,40,255};
       else framebuffer[idx] = (Color){r,g,b,255};
   }
}
