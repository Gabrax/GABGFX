#include "engine.h"
#include "CL/cl.h"
#include "CL/cl_platform.h"
#include "CL/cl_gl.h"
#include <stdint.h>

#define GABMATH_IMPLEMENTATION
#include "gab_math.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <glad/glad.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static cl_platform_id s_platform;
static cl_device_id s_device;
static cl_program s_program;
static cl_context s_context;
static cl_command_queue s_queue;
static cl_int s_err;

static cl_kernel s_clearKernel;
static cl_kernel s_vertexKernel;
static cl_kernel s_fragmentKernel;

static cl_mem s_frameBuffer;
static cl_mem s_depthBuffer;
static cl_mem s_projectedVertsBuffer;
static cl_mem s_fragPosBuffer;
static cl_mem s_projectionBuffer;
static cl_mem s_viewBuffer;
static cl_mem s_cameraPosBuffer;
static cl_mem s_trianglesBuffer;
static cl_mem s_pixelsBuffer;
static cl_mem s_modelsBuffer;

static size_t s_screenResolution[2];

static GLFWwindow* s_window = NULL;
static GLuint gTexture = 0;
static GLuint quadVAO = 0;
static GLuint quadProg = 0;

typedef struct 
{
  f3 Position;
  f3 Front;
  f3 Up;
  f3 Right;
  f3 WorldUp;
  f4x4 proj;
  f4x4 look_at;
  float near_plane;
  float far_plane;
  float fov;
  float fov_rad;
  float aspect_ratio;
  float yaw;
  float pitch;
  float speed;
  float sens;
  float lastX;
  float lastY;
  bool firstMouse;
  float deltaTime;

} CustomCamera;

static CustomCamera s_camera = {0};

typedef struct {
    int triangleOffset;
    int triangleCount;
    int vertexOffset;
    int vertexCount;
    int pixelOffset;
    int texWidth;
    int texHeight;
    f4x4 transform;
} CustomModel;

static Triangle* s_allTriangles = NULL;
static cl_float4* s_allTexturePixels = NULL;
static CustomModel* s_Models = NULL;

static size_t s_totalTriangles = 0;
static size_t s_totalVerts = 0;
static size_t s_totalTexturePixels = 0;
static size_t s_triOffset = 0;
static size_t s_pixOffset = 0;

const char* vs_src = "#version 330 core\nout vec2 uv;\nvoid main(){ vec2 p = vec2((gl_VertexID>>1)*2-1, (gl_VertexID&1)*2-1); gl_Position = vec4(p,0,1); uv = p*0.5 + 0.5; }\n";
const char* fs_src = "#version 330 core\nin vec2 uv; uniform sampler2D tex; out vec4 outCol; void main(){ outCol = texture(tex, uv); }\n";

static const char* engine_load_kernel(const char* filename)
{
  FILE* f = fopen(filename, "rb");
  if(!f) { printf("Cannot open kernel file.\n"); return NULL; }
  fseek(f,0,SEEK_END);
  size_t size = ftell(f);
  fseek(f,0,SEEK_SET);
  char* src = (char*)malloc(size+1);
  fread(src,1,size,f);
  src[size] = '\0';
  fclose(f);
  return src;
}

static GLuint compile_shader_program(const char* vs_src, const char* fs_src)
{
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_src, NULL);
  glCompileShader(vs);
  GLint ok; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
  if (!ok) { char buf[1024]; glGetShaderInfoLog(vs, 1024, NULL, buf); printf("VS compile: %s\n", buf); }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_src, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
  if (!ok) { char buf[1024]; glGetShaderInfoLog(fs, 1024, NULL, buf); printf("FS compile: %s\n", buf); }

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) { char buf[1024]; glGetProgramInfoLog(prog, 1024, NULL, buf); printf("Prog link: %s\n", buf); }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

void engine_init(const char* kernel,int width, int height)
{
  if (!glfwInit())
      exit(1);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  s_window = glfwCreateWindow(width, height, "GABGFX", NULL, NULL);

  glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  int xpos = (mode->width - width) / 2;
  int ypos = (mode->height - height) / 2;

  glfwSetWindowPos(s_window, xpos, ypos);

  glfwMakeContextCurrent(s_window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
      exit(2);

  s_screenResolution[0] = width;
  s_screenResolution[1] = height;

  glfwSwapInterval(1); // V-sync
  
  glGenTextures(1, &gTexture);
  glBindTexture(GL_TEXTURE_2D, gTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_screenResolution[0],
               s_screenResolution[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


  clGetPlatformIDs(1, &s_platform, NULL);
  clGetDeviceIDs(s_platform, CL_DEVICE_TYPE_GPU, 1, &s_device, NULL);

  cl_context_properties props[] = {
      CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(s_window),
      CL_WGL_HDC_KHR,     (cl_context_properties)GetDC(glfwGetWin32Window(s_window)),
      CL_CONTEXT_PLATFORM, (cl_context_properties)s_platform,
      0
  };

  s_context = clCreateContext(props, 1, &s_device, NULL, NULL, NULL);
  s_queue = clCreateCommandQueue(s_context, s_device, 0, NULL);

  s_frameBuffer = clCreateFromGLTexture(s_context,
                                  CL_MEM_WRITE_ONLY,
                                  GL_TEXTURE_2D,
                                  0,
                                  gTexture,
                                  &s_err);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "clCreateFromGLTexture failed: %d\n", s_err); exit(1); } 
  
  const char* kernelSource = engine_load_kernel(kernel); 
  s_program = clCreateProgramWithSource(s_context, 1, &kernelSource, NULL, &s_err);
  if (s_err != CL_SUCCESS) { printf("Error creating program: %d\n", s_err); }

  s_err = clBuildProgram(s_program, 1, &s_device, NULL, NULL, NULL);
  if (s_err != CL_SUCCESS) {
      size_t log_size;
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
      char* log = (char*)malloc(log_size);
      clGetProgramBuildInfo(s_program, s_device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
      printf("OpenCL build error:\n%s\n", log);
      free(log);
  }

  s_clearKernel    = clCreateKernel(s_program, "clear_buffers", &s_err);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "clCreateKernel clear buffer failed: %d\n", s_err); exit(1); }
  s_vertexKernel   = clCreateKernel(s_program, "vertex_kernel", &s_err);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "clCreateKernel vertex failed: %d\n", s_err); exit(1); }
  s_fragmentKernel = clCreateKernel(s_program, "fragment_kernel", &s_err);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "clCreateKernel fragment failed: %d\n", s_err); exit(1); }

  s_depthBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE,
                                      sizeof(cl_uint) * s_screenResolution[0]
                                                    * s_screenResolution[1],
                                                    NULL, &s_err);

  clSetKernelArg(s_clearKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_clearKernel, 1, sizeof(cl_mem), &s_depthBuffer);
  clSetKernelArg(s_clearKernel, 2, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_clearKernel, 3, sizeof(int), &s_screenResolution[1]);
  clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);

  clSetKernelArg(s_vertexKernel, 8, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_vertexKernel, 9, sizeof(int), &s_screenResolution[1]);

  clSetKernelArg(s_fragmentKernel, 0, sizeof(cl_mem), &s_frameBuffer);
  clSetKernelArg(s_fragmentKernel, 2, sizeof(int), &s_screenResolution[0]);
  clSetKernelArg(s_fragmentKernel, 3, sizeof(int), &s_screenResolution[1]);
  clSetKernelArg(s_fragmentKernel, 4, sizeof(cl_mem), &s_depthBuffer);

  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);
  glBindVertexArray(0);
  quadProg = compile_shader_program(vs_src, fs_src);
}

void engine_background_color(cl_float4 color)
{
  clSetKernelArg(s_clearKernel, 4, sizeof(cl_float4), &color);
}

bool engine_is_running()
{
  return !glfwWindowShouldClose(s_window);
}

void engine_start_drawing()
{
  glFinish(); 

  s_err = clEnqueueAcquireGLObjects(s_queue, 1, &s_frameBuffer, 0, NULL, NULL);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "AcquireGLObjects failed: %d\n", s_err); }

  clEnqueueNDRangeKernel(s_queue, s_clearKernel, 2, NULL,
                         s_screenResolution, NULL, 0, NULL, NULL);

  clEnqueueNDRangeKernel(s_queue, s_vertexKernel, 1, NULL,
                         &s_totalVerts, NULL, 0, NULL, NULL);
}

void engine_end_drawing()
{
  s_err = clEnqueueNDRangeKernel(s_queue, s_fragmentKernel, 2, NULL, s_screenResolution, NULL, 0, NULL, NULL);
  if (s_err != CL_SUCCESS) { fprintf(stderr, "Fragment kernel failed: %d\n", s_err); }

  s_err = clEnqueueReleaseGLObjects(s_queue, 1, &s_frameBuffer, 0, NULL, NULL);
  clFinish(s_queue);

  glUseProgram(quadProg);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gTexture);
  glUniform1i(glGetUniformLocation(quadProg, "tex"), 0);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);

  glfwSwapBuffers(s_window);
  glfwPollEvents();
}

void engine_close()
{
  clFinish(s_queue);
  if (s_frameBuffer) clReleaseMemObject(s_frameBuffer);
  if (s_fragmentKernel) clReleaseKernel(s_fragmentKernel);
  if (s_program) clReleaseProgram(s_program);
  if (s_queue) clReleaseCommandQueue(s_queue);
  if (s_context) clReleaseContext(s_context);
  if (s_device) clReleaseDevice(s_device); // opcjonalnie

  if (quadProg) glDeleteProgram(quadProg);
  if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
  if (gTexture) { glDeleteTextures(1, &gTexture); }

  if (s_window) { glfwDestroyWindow(s_window); glfwTerminate(); }
}

bool engine_key_down(int key)
{
  return glfwGetKey(s_window, key) == GLFW_PRESS;
}

void engine_load_model(const char* filePath, const char* texturePath, f4x4 transform)
{
  const struct aiScene* scene = aiImportFile(
      filePath,
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_GenSmoothNormals |
      aiProcess_ImproveCacheLocality |
      aiProcess_OptimizeMeshes
  );

  if (!scene || scene->mNumMeshes == 0) {
      fprintf(stderr, "Failed to load model: %s\n", filePath);
      aiReleaseImport(scene);
      return;
  }

  Triangle* triangles = NULL;
  size_t numTriangles = 0;
  size_t numVertices = 0;

  int modelIndex = arrlen(s_Models);

  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
      const struct aiMesh* mesh = scene->mMeshes[m];

      for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
          const struct aiFace* face = &mesh->mFaces[f];
          if (face->mNumIndices != 3) continue; // skip non-triangles

          Triangle tri = {0};
          for (int i = 0; i < 3; i++) {
              unsigned int idx = face->mIndices[i];

              if (mesh->mVertices) {
                  tri.vertex[i].x = mesh->mVertices[idx].x;
                  tri.vertex[i].y = mesh->mVertices[idx].y;
                  tri.vertex[i].z = mesh->mVertices[idx].z;
              }
              if (mesh->mNormals) {
                  tri.normal[i].x = mesh->mNormals[idx].x;
                  tri.normal[i].y = mesh->mNormals[idx].y;
                  tri.normal[i].z = mesh->mNormals[idx].z;
              }
              if (mesh->mTextureCoords[0]) {
                  tri.uv[i].x = mesh->mTextureCoords[0][idx].x;
                  tri.uv[i].y = mesh->mTextureCoords[0][idx].y;
              }
          }
          
          tri.modelIdx = modelIndex;

          arrpush(triangles, tri);
          numTriangles++;
          numVertices += 3;
      }
  }

  aiReleaseImport(scene);

  int texWidth = 0, texHeight = 0;
  cl_float4* pixels = NULL;

  /*if (texturePath) {*/
  /*    Image img = LoadImage(texturePath);*/
  /*    texWidth = img.width;*/
  /*    texHeight = img.height;*/
  /**/
  /*    if (texWidth > 0 && texHeight > 0) {*/
  /*        pixels = (Color*)malloc(texWidth * texHeight * sizeof(Color));*/
  /*        memcpy(pixels, img.data, texWidth * texHeight * sizeof(Color));*/
  /*    }*/
  /**/
  /*    UnloadImage(img); // free Raylib image memory*/
  /*}*/

  for (size_t t = 0; t < numTriangles; t++)
      arrpush(s_allTriangles, triangles[t]);

  /*if (pixels) {*/
  /*    size_t numPixels = texWidth * texHeight;*/
  /*    for (size_t p = 0; p < numPixels; p++)*/
  /*        arrpush(s_allTexturePixels, pixels[p]);*/
  /*    free(pixels);*/
  /*}*/

  CustomModel m;
  m.triangleOffset = s_triOffset;
  m.triangleCount  = (int)numTriangles;
  m.vertexOffset   = 0;
  m.vertexCount    = (int)numVertices;
  m.pixelOffset    = s_pixOffset;
  m.texWidth       = texWidth;
  m.texHeight      = texHeight;
  m.transform      = transform;
  arrpush(s_Models, m);

  s_triOffset += numTriangles;
  s_pixOffset += texWidth * texHeight;
  s_totalTriangles += numTriangles;
  s_totalTexturePixels += texWidth * texHeight;

  arrfree(triangles);
}

void engine_upload_models_data()
{  
  int numModels = arrlen(s_Models);
  s_totalVerts = s_totalTriangles * 3;

  s_trianglesBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_allTriangles) * sizeof(Triangle), s_allTriangles, &s_err);

  s_pixelsBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_allTexturePixels) * sizeof(cl_float4), s_allTexturePixels, &s_err);

  s_modelsBuffer = clCreateBuffer(s_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        arrlen(s_Models) * sizeof(CustomModel), s_Models, &s_err);

  s_projectedVertsBuffer = clCreateBuffer(s_context, CL_MEM_READ_WRITE,
                                          sizeof(f4) * s_totalVerts, NULL, NULL);

  clSetKernelArg(s_vertexKernel, 4, sizeof(cl_mem), &s_projectedVertsBuffer);
  clSetKernelArg(s_fragmentKernel, 1, sizeof(cl_mem), &s_projectedVertsBuffer);

  clSetKernelArg(s_vertexKernel, 0, sizeof(cl_mem), &s_trianglesBuffer);
  clSetKernelArg(s_vertexKernel, 1, sizeof(cl_mem), &s_modelsBuffer);
  clSetKernelArg(s_vertexKernel, 2, sizeof(int), &numModels);
  clSetKernelArg(s_vertexKernel, 3, sizeof(int), &s_totalVerts);

  clSetKernelArg(s_fragmentKernel, 6, sizeof(cl_mem), &s_trianglesBuffer);
  clSetKernelArg(s_fragmentKernel, 7, sizeof(cl_mem), &s_modelsBuffer);
  clSetKernelArg(s_fragmentKernel, 8, sizeof(int), &numModels);
  clSetKernelArg(s_fragmentKernel, 9, sizeof(cl_mem), &s_pixelsBuffer);
}

void engine_print_model_data()
{
    for (size_t m = 0; m < arrlen(s_Models); m++) {
        CustomModel* model = &s_Models[m];
        printf("Model %zu:\n", m);
        printf("  Triangles: %d\n", model->triangleCount);
        printf("  Texture size: %dx%d\n", model->texWidth, model->texHeight);
        printf("  Transform matrix:\n");
        MatPrint(&model->transform); 

        for (int t = 0; t < model->triangleCount; t++) {
            const Triangle* tri = &s_allTriangles[model->triangleOffset + t];
            printf("  Triangle %d:\n", t);
            for (int v = 0; v < 3; v++) {
                printf("    Vertex %d:\n", v);
                printf("      Position: (%f, %f, %f)\n",
                       tri->vertex[v].x,
                       tri->vertex[v].y,
                       tri->vertex[v].z);
                printf("      UV:       (%f, %f)\n",
                       tri->uv[v].x,
                       tri->uv[v].y);
                printf("      Normal:   (%f, %f, %f)\n",
                       tri->normal[v].x,
                       tri->normal[v].y,
                       tri->normal[v].z);
            }
        }
        printf("\n");
    }
}

void engine_free_all_models()
{
  arrfree(s_allTriangles);
  arrfree(s_allTexturePixels);
  arrfree(s_Models);
  s_triOffset = 0;
  s_pixOffset = 0;
  s_totalTriangles = 0;
  s_totalTexturePixels = 0;
}

void engine_init_camera(int width, int height, float fov, float near_plane, float far_plane)
{
  s_camera.Position = (f3){0.0f, 0.0f, 0.0f};
  s_camera.WorldUp = (f3){0.0f, 1.0f, 0.0f};
  s_camera.Front = (f3){0.0f, 0.0f, 1.0f};
  s_camera.aspect_ratio = (float)width / (float)height;
  s_camera.near_plane = near_plane;
  s_camera.far_plane = far_plane;
  s_camera.fov = fov;
  s_camera.fov_rad = DegToRad(s_camera.fov);  
  s_camera.proj = MatPerspective(s_camera.fov_rad, s_camera.aspect_ratio, s_camera.near_plane, s_camera.far_plane);
  s_camera.look_at = MatLookAt(s_camera.Position, f3Add(s_camera.Position, s_camera.Front), s_camera.WorldUp);
  s_camera.yaw = 90.0f;
  s_camera.pitch = 0.0f;
  s_camera.speed = 2.0f;
  s_camera.sens = 0.1f;
  s_camera.lastX = width / 2.0f;
  s_camera.lastY = height / 2.0f;
  s_camera.firstMouse = true;
  s_camera.deltaTime = 1.0/60.0f;

  s_projectionBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f4x4), NULL, &s_err);
  s_viewBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f4x4), NULL, &s_err);
  s_cameraPosBuffer  = clCreateBuffer(s_context, CL_MEM_READ_ONLY, sizeof(f3), NULL, &s_err);

  clSetKernelArg(s_vertexKernel, 5, sizeof(cl_mem), &s_projectionBuffer); 
  clSetKernelArg(s_vertexKernel, 6, sizeof(cl_mem), &s_viewBuffer); 
  clSetKernelArg(s_vertexKernel, 7, sizeof(cl_mem), &s_cameraPosBuffer);

  clSetKernelArg(s_fragmentKernel, 5, sizeof(cl_mem), &s_cameraPosBuffer);

  clEnqueueWriteBuffer(s_queue, s_projectionBuffer, CL_TRUE, 0, sizeof(f4x4), &s_camera.proj, 0, NULL, NULL);
}

void engine_process_camera_keys(Movement direction)
{
  float velocity = s_camera.speed * s_camera.deltaTime;

  if (direction == FORWARD) s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Front, velocity));
  if (direction == BACKWARD) s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Front, -velocity));
  if (direction == LEFT) s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Right, velocity));
  if (direction == RIGHT) s_camera.Position = f3Add(s_camera.Position, f3MulS(s_camera.Right, -velocity));
}
void engine_update_camera()
{
  double mouseX, mouseY;
  glfwGetCursorPos(s_window, &mouseX, &mouseY);

  bool constrainPitch = true;

  float xoffset, yoffset;
  if (s_camera.firstMouse)
  {
      s_camera.lastX = mouseX;
      s_camera.lastY = mouseY;
      s_camera.firstMouse = false;
  }

  xoffset = s_camera.lastX - mouseX;
  yoffset = s_camera.lastY - mouseY; 

  s_camera.lastX = mouseX;
  s_camera.lastY = mouseY;

  xoffset *= s_camera.sens;
  yoffset *= s_camera.sens;

  s_camera.yaw   += xoffset;
  s_camera.pitch += yoffset;

  if (constrainPitch)
  {
      if (s_camera.pitch > 89.0f)  s_camera.pitch = 89.0f;
      if (s_camera.pitch < -89.0f) s_camera.pitch = -89.0f;
  }

  f3 front;
  front.x = cosf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
  front.y = sinf(DegToRad(s_camera.pitch));
  front.z = sinf(DegToRad(s_camera.yaw)) * cosf(DegToRad(s_camera.pitch));
  s_camera.Front = f3Norm(front);

  s_camera.Right = f3Norm(f3Cross(s_camera.Front, s_camera.WorldUp));
  s_camera.Up    = f3Norm(f3Cross(s_camera.Right, s_camera.Front));

  s_camera.look_at = MatLookAt(s_camera.Position, f3Add(s_camera.Position, s_camera.Front), s_camera.Up);

  clEnqueueWriteBuffer(s_queue, s_cameraPosBuffer, CL_TRUE, 0,
                       sizeof(f3), &s_camera.Position, 0, NULL, NULL);
  clEnqueueWriteBuffer(s_queue, s_viewBuffer, CL_TRUE, 0,
                       sizeof(f4x4), &s_camera.look_at, 0, NULL, NULL);
}
