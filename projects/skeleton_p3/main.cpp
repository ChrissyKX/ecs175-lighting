//===========================================================================//
//                                                                           //
// Copyright(c) ECS 175 (2020)                                               //
// University of California, Davis                                           //
// MIT Licensed                                                              //
//                                                                           //
//===========================================================================//

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// import imgui
#include "imgui.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
bool imgui_enabled = false;

#include "shaders.h"
#include "util.hpp"
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;

vec3 ka; // ka, kd, ks
vec3 kd;
vec3 ks;

vec2 i; // Ia, Il
vec3 x; // position of the light source
int n; // Phong constant

int gid = 0;
bool ortho_proj = false;
bool half_toning = false;
bool enable_painter_algorithm = false;

#include "geometry_triangle.h"
TriangleArrayObjects* objects;

#include "arcball_camera.h"
ArcballCamera camera(vec3(0, 0, 5), vec3(0, 0, 0), vec3(0, 1, 0));

// Reference: https://www.khronos.org/opengl/wiki/Framebuffer_Object
struct FrameBufferObject {
  GLuint framebuffer_id = 0;
  // Texture which will contain the RGB output of our shader.
  GLuint color_tex;
  // An optional depth buffer. This enables depth-testing.
  GLuint depth_tex; // depth texture. slower, but you can sample it later in your shader
  GLuint depth_rb; // depth render buffer: faster

  void
  Generate()
  {
    glGenFramebuffers(1, &framebuffer_id);
    glGenTextures(1, &color_tex);
    glGenTextures(1, &depth_tex);
    glGenRenderbuffers(1, &depth_rb);
  }

  void
  Resize(int width, int height, bool enable_depth_texture)
  {
    BindFramebuffer();

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, color_tex);
    {
      // Give an empty image to OpenGL ( the last "0" means "empty" )
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

      // Poor filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex, 0);

    // The depth buffer
    if (!enable_depth_texture) {
      glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
    }
    else {
      glBindTexture(GL_TEXTURE_2D, depth_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_tex, 0);
    }

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("framebuffer object incomplete");

    // Switch back to the default framebuffer
    UnbindFramebuffer();
  }

  void
  BindFramebuffer()
  {
    // Switch to the framebuffer object
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
  }

  void
  UnbindFramebuffer()
  {
    // Switch back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void
  BindColorTexture()
  {
    glBindTexture(GL_TEXTURE_2D, color_tex);
  }

  void
  BindDepthTexture()
  {
    glBindTexture(GL_TEXTURE_2D, depth_tex);
  }

  void
  Clear()
  {
    glDeleteFramebuffers(1, &framebuffer_id);
    glDeleteTextures(1, &color_tex);
    glDeleteTextures(1, &depth_tex);
    glDeleteRenderbuffers(1, &depth_rb);
  }
};

FrameBufferObject fbo;

void
cursor(GLFWwindow* window, double xpos, double ypos)
{
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    int left_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    static vec2 prev_cursor;
    vec2 cursor((xpos / width - 0.5f) * 2.f, (0.5f - ypos / height) * 2.f);

    // right click -> zoom
    if (right_state == GLFW_PRESS || right_state == GLFW_REPEAT) {
      camera.zoom(cursor.y - prev_cursor.y);
    }

    // left click -> rotate
    if (left_state == GLFW_PRESS || left_state == GLFW_REPEAT) {
      camera.rotate(prev_cursor, cursor);
    }

    prev_cursor = cursor;
  }
}

static void
KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    // close window
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  else if (key == GLFW_KEY_G && action == GLFW_PRESS) {
    imgui_enabled = !imgui_enabled;
  }
}

void
init()
{
  // -----------------------------------------------------------
  // For reference only, feel free to make changes
  // -----------------------------------------------------------

  // Initialise GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  const char* glsl_version = "#version 150"; // GL 3.3 + GLSL 150
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow(1024, 768, "ECS 175 (press 'g' to display GUI)", NULL, NULL);
  if (window == NULL) {
    glfwTerminate();
    throw std::runtime_error("Failed to open GLFW window. If you have a GPU that is "
                             "not 3.3 compatible, try a lower OpenGL version.");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Load GLAD symbols
  int err = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0;
  if (err) {
    throw std::runtime_error("Failed to initialize OpenGL loader!");
  }

  glfwSetCursorPosCallback(window, cursor);
  glfwSetKeyCallback(window, KeyCallback);

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  // Dark blue background (avoid using black)
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);
}

// Compute the order of the triangles
void
PainterAlgorithm(mat4 mv)
{
  for (auto& mesh : objects->meshes) {
    auto v = mesh.vertices.get();

    // loop through triangles
    mesh.painter_tri_order.resize(mesh.size_triangles, std::pair<int, float>(0, 0.f));
    for (int i = 0; i < mesh.size_triangles; i++) {
      // find the minimum z
      vec4 v1(v[i * 9 + 0], v[i * 9 + 1], v[i * 9 + 2], 1);
      vec4 v2(v[i * 9 + 3], v[i * 9 + 4], v[i * 9 + 5], 1);
      vec4 v3(v[i * 9 + 6], v[i * 9 + 7], v[i * 9 + 8], 1);
      v1 = mv * v1, v2 = mv * v2, v3 = mv * v3;
      auto min_depth = std::max(std::max(v1.z, v2.z), v3.z);
      mesh.painter_tri_order[i].first = i;
      mesh.painter_tri_order[i].second = min_depth;
    }

    sort(mesh.painter_tri_order.begin(),
         mesh.painter_tri_order.end(),
         [](const std::pair<int, float>& a, const std::pair<int, float>& b) -> bool { return a.second < b.second; });
  }

  sort(objects->meshes.begin(),
       objects->meshes.end(),
       [](const TriangleArrayObjects::Mesh& a, const TriangleArrayObjects::Mesh& b) -> bool {
         return a.painter_tri_order[0].second < b.painter_tri_order[0].second;
       });
}

void
BindAndRender(const mat4& trans = mat4(1.f))
{
  if (!enable_painter_algorithm) {
    glEnable(GL_DEPTH_TEST);
    objects->Create();
    objects->BindWholeBuffer();
  }
  else {
    glDisable(GL_DEPTH_TEST);
    for (auto& m : objects->meshes)
      PainterAlgorithm(trans);
  }
  objects->Render();
}

void RenderDivider(GLint width, GLint height, GLint divider_vertexbuffer_id, GLuint divider_id) {
  glUniform1i(divider_id, true);
  glViewport(0, 0, width, height);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, divider_vertexbuffer_id);
  glVertexAttribPointer(0, // attribute
                        3, // size
                        GL_FLOAT, // type
                        GL_FALSE, // normalized?
                        0, // stride
                        (void*)0 // array buffer offset
  );

  glDrawArrays(GL_LINES, 0, 4);
  glDisableVertexAttribArray(0);
  glUniform1i(divider_id, false);
}

void
initImgui()
{
  // ImGui
  {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // or ImGui::StyleColorsClassic();

    // Initialize Dear ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true /* 'true' -> allow imgui to capture keyboard inputs */);
    const char* glsl_version = "#version 150"; // GL 3.3 + GLSL 150
    ImGui_ImplOpenGL3_Init(glsl_version);
  }
}

void
gui(const char* filename)
{
  if (imgui_enabled) {
    // Initialization
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Graphical User Interface");

    ImGui::Text("Object File: %s", filename);

    ImGui::Checkbox("Enable Othorgraphic Projection", &ortho_proj);

    ImGui::Checkbox("Enable Painter's Algorithm", &enable_painter_algorithm);

    ImGui::Checkbox("Enable Half-Toning", &half_toning);

    if (ImGui::CollapsingHeader("Phong Lighting Model Parameters")) {
      ImGui::Text("Intensities (You may want to set Ambient Intensity much smaller than\nLight Source Intensity to "
                  "obtaina reasonable lighting)");
      ImGui::InputFloat("Ambient##A1", &(i.x), 0.1f, 0.1f, "%.02f");
      ImGui::InputFloat("Light Source", &(i.y), 0.1f, 0.1f, "%.02f");
      ImGui::NewLine();
      ImGui::Text("Reflection Coefficients [0, 1]");
      ImGui::InputFloat3("Ambient##A2", &ka[0], 2);
      ImGui::InputFloat3("Diffuse", &kd[0], 2);
      ImGui::InputFloat3("Specular", &ks[0], 2);
      ImGui::NewLine();
      ImGui::Text("Position of Light Source");
      ImGui::InputFloat3("(x, y, z)", &x[0], 2);
      ImGui::NewLine();
      ImGui::Text("Phong Constant (Positive Integer)");
      ImGui::InputInt("", &n);
    }

    ImGui::End();

    // Render GUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
}

void
cleanImgui()
{
  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

int
main(int argc, char** argv)
{
  init();
  initImgui();

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create and compile our GLSL program from the shaders
  GLuint program_id = LoadProgram_FromEmbededTexts((char*)vshader, vshader_size, (char*)fshader, fshader_size);
  GLuint program_quad =
    LoadProgram_FromEmbededTexts((char*)vshader_quad, vshader_quad_size, (char*)fshader_quad, fshader_quad_size);

  // Initialize global variables
  ka = vec3(0.7, 0.5, 0.5);
  kd = vec3(0.7, 0.5, 0.5);
  ks = vec3(0.7, 0.6, 0.5);
  i = vec2(0.5, 20); // Ia, Il
  x = vec3(20, 20, 20); // position of light source
  n = 3;

  // Get a handle for our uniforms in vertex shader
  GLuint MVP_id = glGetUniformLocation(program_id, "MVP");
  GLuint ka_id = glGetUniformLocation(program_id, "ka");
  GLuint kd_id = glGetUniformLocation(program_id, "kd");
  GLuint ks_id = glGetUniformLocation(program_id, "ks");
  GLuint i_id = glGetUniformLocation(program_id, "i");
  GLuint x_id = glGetUniformLocation(program_id, "x");
  GLuint f_id = glGetUniformLocation(program_id, "f");
  GLuint n_id = glGetUniformLocation(program_id, "n");
  GLuint divider_id = glGetUniformLocation(program_id, "divider");

  // Get a handle for our uniforms in fragment shader
  GLuint renderedTexture_id = glGetUniformLocation(program_quad, "renderedTexture");

  // Load the texture
  GLuint tex = loadTexture_from_file("uvmap.jpg");

  // Read our .obj file
  objects = ReadAsArrayObjects(argv[1]);
  objects->Create();

  // Render original scene to a texture width/3 * height/3
  fbo.Generate();
  ivec2 framebuffer_size;
  glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);
  fbo.Resize(framebuffer_size.x / 3, framebuffer_size.y / 3, false);

  // The fullscreen quad's FBO
  static const GLfloat g_quad_vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
  };

  GLuint quad_vertexbuffer;
  glGenBuffers(1, &quad_vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

  // Orthographic projection divider
  GLuint divider_vertexbuffer;
  glGenBuffers(1, &divider_vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, divider_vertexbuffer);
  static const GLfloat divider_data[] = {0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, -1.f, 0.f, 0.f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, divider_data, GL_STATIC_DRAW);

  do {

    if (half_toning) {
      // Render to our framebuffer
      fbo.BindFramebuffer();
      {
        // Clear the screen

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Send our transformation to the currently bound shader, in the "MVP" uniform
        mat4 P = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
        mat4 M = objects->GetModelMatrix();
        mat4 V = camera.transform();
        mat4 MVP = P * V * M;

        // Use the rendering pass
        glUseProgram(program_id);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &MVP[0][0]);
        glUniform3fv(ka_id, 1, &ka[0]);
        glUniform3fv(kd_id, 1, &kd[0]);
        glUniform3fv(ks_id, 1, &ks[0]);
        glUniform2fv(i_id, 1, &i[0]);
        glUniform3fv(x_id, 1, &x[0]);
        glUniform3fv(f_id, 1, &camera.eye()[0]);
        glUniform1i(n_id, n);
        glUniform1i(divider_id, false);

        if (!ortho_proj) {
          glViewport(0, 0, framebuffer_size.x / 3, framebuffer_size.y / 3);
          BindAndRender(V * M);
        }
        else {
          RenderDivider(framebuffer_size.x / 3, framebuffer_size.y / 3, divider_vertexbuffer, divider_id);

          glViewport(
            0, framebuffer_size.y / 6, framebuffer_size.x / 6, framebuffer_size.y / 6); // projection onto xy-plane
          mat4 MVO = V * M;
          glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
          BindAndRender(MVO);

          glViewport(0, 0, framebuffer_size.x / 6, framebuffer_size.y / 6); // projection onto xz-plane
          MVO = rotate(V * M, radians(90.f), vec3(1.f, 0.f, 0.f));
          glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
          BindAndRender(MVO);

          glViewport(framebuffer_size.x / 6,
                     framebuffer_size.y / 6,
                     framebuffer_size.x / 6,
                     framebuffer_size.y / 6); // projection onto yz-plane
          MVO = rotate(V * M, radians(-90.f), vec3(0.f, 1.f, 0.f));
          glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
          BindAndRender(MVO);
        }
      }
      fbo.UnbindFramebuffer();

      // Render to the screen
      glViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
      {
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        // Use our shader
        glUseProgram(program_quad);

        glActiveTexture(GL_TEXTURE0); // Bind our texture in Texture Unit 0
        glUniform1i(renderedTexture_id, 0); // Set our "renderedTexture" sampler to use Texture Unit 0
        fbo.BindColorTexture();

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
        glVertexAttribPointer(0, // attribute 0. No particular reason for 0, but must match the layout in the shader.
                              3, // size
                              GL_FLOAT, // type
                              GL_FALSE, // normalized?
                              0, // stride
                              (void*)0 // array buffer offset
        );

        // Draw the triangles for quad!
        glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
        glDisableVertexAttribArray(0);
      }
    }
    else {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);

      // Send our transformation to the currently bound shader, in the "MVP" uniform
      mat4 P = perspective(radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
      mat4 M = objects->GetModelMatrix();
      mat4 V = camera.transform();
      mat4 MVP = P * V * M;

      // Use the rendering pass
      glUseProgram(program_id);

      // Send our transformation to the currently bound shader,
      // in the "MVP" uniform
      glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &MVP[0][0]);
      glUniform3fv(ka_id, 1, &ka[0]);
      glUniform3fv(kd_id, 1, &kd[0]);
      glUniform3fv(ks_id, 1, &ks[0]);
      glUniform2fv(i_id, 1, &i[0]);
      glUniform3fv(x_id, 1, &x[0]);
      glUniform3fv(f_id, 1, &camera.eye()[0]);
      glUniform1i(n_id, n);

      if (!ortho_proj) {
        glViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
        BindAndRender(V * M);
      }
      else {
        RenderDivider(framebuffer_size.x, framebuffer_size.y, divider_vertexbuffer, divider_id);

        glViewport(
          0, framebuffer_size.y / 2, framebuffer_size.x / 2, framebuffer_size.y / 2); // projection onto xy-plane
        mat4 MVO = V * M;
        glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
        BindAndRender(MVO);

        glViewport(0, 0, framebuffer_size.x / 2, framebuffer_size.y / 2); // projection onto xz-plane
        MVO = rotate(V * M, radians(90.f), vec3(1.f, 0.f, 0.f));
        glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
        BindAndRender(MVO);

        glViewport(framebuffer_size.x / 2,
                   framebuffer_size.y / 2,
                   framebuffer_size.x / 2,
                   framebuffer_size.y / 2); // projection onto yz-plane
        MVO = rotate(V * M, radians(-90.f), vec3(0.f, 1.f, 0.f));
        glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(P * MVO)[0][0]);
        BindAndRender(MVO);
      }
    }

    gui(argv[1]);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

  // Cleanup VBO and shader
  objects->Clear();
  glDeleteProgram(program_id);
  glDeleteTextures(1, &tex);
  glDeleteVertexArrays(1, &vao);

  // Clean ImGui
  cleanImgui();

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  delete objects;

  return 0;
}
