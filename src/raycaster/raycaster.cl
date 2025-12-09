

__kernel void fragment_kernel(
    __global Pixel* pixels,
    int width,
    int height,
    __global float* depthBuffer,
    __global float2 playerPos,
    __global Pixel* textures)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  int idx = y * width + x;
  float2 P = (float2)(x + 0.5f, y + 0.5f); // pixel center

  for (int y=0;y<(height/2)+1;++y)
  {
    for (int x=0;x<width;++x)
    {

    }
  }

  pixels[idx] = (Pixel){
      (uchar)(finalColor.x * 255),
      (uchar)(finalColor.y * 255),
      (uchar)(finalColor.z * 255),
      255
  };

  depthBuffer[idx] = depth;
}
