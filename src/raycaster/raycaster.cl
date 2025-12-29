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

typedef struct GameData {
  size_t map_data_size;
  uint8* map_data;
  SpriteData* sprites;
} GameData;

typedef struct Sprite {
  int pixelOffset;
  int texWidth;
  int texHeight;
} Sprite;

typedef struct Player {
  float2 pos;
  float2 dir;
  float2 projection;
  float movespeed;
  bool is_shooting;
  size_t sprite_index;
  size_t frame_counter;
} Player;

__kernel void clear_buffers(
    __global Pixel* pixels,
    __global float* depth,
    int width, int height,
    Pixel color)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;
    int idx = y * width + x;
    pixels[idx] = color;
    depth[idx] = FLT_MAX;
}

__kernel void fragment_kernel(
    __global Pixel* pixels,
    int width,
    int height,
    __global float* depthBuffer,
    __global Player* player,
    __global GameData* game_data,
    __global Pixel* textures,
    __global Sprite* sprites)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  int idx = (height - y - 1) * (width + x);
  float2 P = (float2)(x + 0.5f, y + 0.5f); // pixel center

  for (int y=0;y<(height/2)+1;++y)
  {
    for (int x=0;x<width;++x)
    {

    }
  }

  float depth = 0;
  float3 finalColor = 1;

  pixels[idx] = (Pixel){
      (uchar)(finalColor.x * 255),
      (uchar)(finalColor.y * 255),
      (uchar)(finalColor.z * 255),
      255
  };

  depthBuffer[idx] = depth;
}

    /*fn render_floor_ceiling(&mut self) {*/
    /*    let mut floor_texture = self.textures[6].borrow_mut();  // Borrow the image immutably*/
    /*    let mut ceiling_texture = self.textures[1].borrow_mut();  // Borrow the image immutably*/
    /**/
    /*    let player = self.player.borrow();*/
    /**/
    /*    let ray_dir_x0 = player.dir.x - player.projection.x;*/
    /*    let ray_dir_y0 = player.dir.y - player.projection.y;*/
    /*    let ray_dir_x1 = player.dir.x + player.projection.x;*/
    /*    let ray_dir_y1 = player.dir.y + player.projection.y;*/
    /**/
    /*    let pos_z = 0.5 * self.buffer_height as f32;*/
    /**/
    /*    for y in (self.buffer_height / 2 + 1)..self.buffer_height {*/
    /*        let p = y as f32 - self.buffer_height as f32 / 2.0;*/
    /*        let row_distance = pos_z / p;*/
    /**/
    /*        let floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / self.buffer_width as f32;*/
    /*        let floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / self.buffer_width as f32;*/
    /**/
    /*        let mut floor_x = player.pos.x + row_distance * ray_dir_x0;*/
    /*        let mut floor_y = player.pos.y + row_distance * ray_dir_y0;*/
    /**/
    /*        for x in 0..self.buffer_width {*/
    /*            let cell_x = floor_x as i32;*/
    /*            let cell_y = floor_y as i32;*/
    /**/
    /*            let tex_x = ((floor_x - cell_x as f32) * floor_texture.width as f32) as i32 & (floor_texture.width - 1);*/
    /*            let tex_y = ((floor_y - cell_y as f32) * floor_texture.height as f32) as i32 & (floor_texture.height - 1);*/
    /**/
    /*            floor_x += floor_step_x;*/
    /*            floor_y += floor_step_y;*/
    /**/
    /*            let floor_color = floor_texture.get_color(tex_x, tex_y);*/
    /*            // Store the floor pixel color in pixelbuffer*/
    /*            self.pixelbuffer[(y * self.buffer_width + x) as usize] = Self::color_to_u32(floor_color);*/
    /**/
    /*            let ceiling_color = ceiling_texture.get_color(tex_x, tex_y);*/
    /*            // Store the ceiling pixel color in pixelbuffer (mirrored y-coordinate)*/
    /*            self.pixelbuffer[((self.buffer_height - y - 1) * self.buffer_width + x) as usize] = Self::color_to_u32(ceiling_color);*/
    /*        }*/
    /*    }*/
    /*}*/


