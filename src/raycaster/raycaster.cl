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
typedef struct Sprite { int offset; int width; int height; } Sprite;

inline Color sample_sprite(
    __global Color* atlas,
    __global Sprite* sprites,
    int sprite_id,
    int x, int y)
{
    Sprite s = sprites[sprite_id];
    x = clamp(x, 0, s.width - 1);
    y = clamp(y, 0, s.height - 1);
    return atlas[s.offset + y * s.width + x];
}

__kernel void fragment_kernel(
   __global Color* framebuffer,
   int screen_width,
   int screen_height,
   __global Player* player,
   __global uchar* map_data,
   int map_size,
   __global Color* texture_atlas,
   __global Sprite* sprites)
{
    int x = get_global_id(0);
    if(x >= screen_width) return;

    Player p = player[0];

    float cameraX = 2.0f * x / (float)screen_width - 1.0f;
    float rayDirX = p.dirX + p.planeX * cameraX;
    float rayDirY = p.dirY + p.planeY * cameraX;

    int mapX = (int)p.x;
    int mapY = (int)p.y;

    float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabs(1.0f / rayDirX);
    float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabs(1.0f / rayDirY);

    int stepX = (rayDirX < 0) ? -1 : 1;
    int stepY = (rayDirY < 0) ? -1 : 1;

    float sideDistX = (rayDirX < 0) ? (p.x - mapX) * deltaDistX : (mapX + 1.0f - p.x) * deltaDistX;
    float sideDistY = (rayDirY < 0) ? (p.y - mapY) * deltaDistY : (mapY + 1.0f - p.y) * deltaDistY;

    int hit = 0;
    int side = 0;
    int wall_id = 0;

    while(!hit)
    {
        if(sideDistX < sideDistY)
        {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        }
        else
        {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }

        if(mapX < 0 || mapY < 0 || mapX >= map_size || mapY >= map_size) break;

        wall_id = map_data[mapY * map_size + mapX];
        if(wall_id > 0) hit = 1;
    }

    float perpWallDist = (side == 0)
        ? (mapX - p.x + (1 - stepX) * 0.5f) / rayDirX
        : (mapY - p.y + (1 - stepY) * 0.5f) / rayDirY;
    perpWallDist = fmax(perpWallDist, 0.01f);

    int lineHeight = (int)(screen_height / perpWallDist);
    int drawStart = max(-lineHeight / 2 + screen_height / 2, 0);
    int drawEnd   = min(lineHeight / 2 + screen_height / 2, screen_height - 1);

    int floor_tex = 1;
    int ceiling_tex = 0;

    for(int y = 0; y < screen_height; y++)
    {
        int idx = y * screen_width + x;

        if(y < drawStart)
        {
            // Ceiling
            float rowDist = screen_height / (2.0f * y - screen_height);
            float worldX = p.x + (-rayDirX) * rowDist;
            float worldY = p.y + (-rayDirY) * rowDist;

            Sprite s = sprites[ceiling_tex];
            int texX = (int)((worldX - floor(worldX)) * s.width);
            int texY = (int)((worldY - floor(worldY)) * s.height);
            framebuffer[idx] = sample_sprite(texture_atlas, sprites, ceiling_tex, texX, texY);
        }
        else if(y > drawEnd)
        {
            // Floor
            float rowDist = screen_height / (2.0f * y - screen_height);
            float worldX = p.x + rayDirX * rowDist;
            float worldY = p.y + rayDirY * rowDist;

            Sprite s = sprites[floor_tex];
            int texX = (int)((worldX - floor(worldX)) * s.width);
            int texY = (int)((worldY - floor(worldY)) * s.height);
            framebuffer[idx] = sample_sprite(texture_atlas, sprites, floor_tex, texX, texY);
        }
        else
        {
            // Wall
            int tex_id;
            switch(wall_id)
            {
                case 1: tex_id = 2; break;
                case 2: tex_id = 3; break;
                case 3: tex_id = 4; break;
                case 4: tex_id = 5; break;
                case 5: tex_id = 6; break;
                default: tex_id = 2; break;
            }

            float wallX = (side == 0)
                ? p.y + perpWallDist * rayDirY
                : p.x + perpWallDist * rayDirX;
            wallX -= floor(wallX);

            Sprite s = sprites[tex_id];
            int texX = (int)(wallX * s.width);
            if(side == 0 && rayDirX > 0) texX = s.width - texX - 1;
            if(side == 1 && rayDirY < 0) texX = s.width - texX - 1;

            int d = y * 256 - screen_height * 128 + lineHeight * 128;
            int texY = ((d * s.height) / lineHeight) / 256;

            framebuffer[idx] = sample_sprite(texture_atlas, sprites, tex_id, texX, texY);
        }
    }
}
