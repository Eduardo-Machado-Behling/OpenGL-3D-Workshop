#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

// IMGUI HEADERS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// CUSTOM HEADERS
#include "camera.h"
#include "shader.h"

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);

// --- Global Variables ---
// Window
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// Camera
Camera camera(glm::vec3(0.0f, 15.0f, 55.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool ui_capture_mouse = false; // Start with camera control enabled

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Cube Data
struct CubeInstance {
  glm::mat4 modelMatrix;
};
std::vector<CubeInstance> cubes;
const int NUM_CUBES = 1;

// Rendering Method
enum RenderMethod { UNIFORM, INSTANCED, INDIRECT };
RenderMethod currentMethod = INSTANCED;

// --- Main Function ---
int main() {
  // --- GLFW/GLAD Initialization ---
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(
      SCR_WIDTH, SCR_HEIGHT, "OpenGL Performance Demo with ImGui", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // --- IMGUI SETUP ---
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 450");

  // --- OpenGL Global State ---
  glEnable(GL_DEPTH_TEST);

  // --- Shader Setup ---
  Shader uniformShader("shaders/uniform_cube.vert", "shaders/cube.frag");
  Shader instancedShader("shaders/instanced_cube.vert", "shaders/cube.frag");
  Shader gridShader("shaders/grid.vert", "shaders/grid.frag");

  // --- Cube Geometry ---
  float vertices[] = {
      // positions          // normals
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
      0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
      0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
      0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
      0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
      0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};
  unsigned int cubeVAO, cubeVBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  // --- Grid Geometry ---
  unsigned int xzPlaneVAO, xzPlaneVBO; // Green
  unsigned int xyPlaneVAO, xyPlaneVBO; // Blue
  unsigned int yzPlaneVAO, yzPlaneVBO; // Red

  // --- Grid/Plane Geometry ---
  const int gridSize = 50;
  const float gridHalfSize = (float)gridSize;

  // --- XZ Plane (Y=0, Green) ---
  std::vector<float> xz_vertices;
  for (int i = -gridSize; i <= gridSize; ++i) {
    xz_vertices.push_back((float)i);
    xz_vertices.push_back(0.0f);
    xz_vertices.push_back(-gridHalfSize);
    xz_vertices.push_back((float)i);
    xz_vertices.push_back(0.0f);
    xz_vertices.push_back(gridHalfSize);
    xz_vertices.push_back(-gridHalfSize);
    xz_vertices.push_back(0.0f);
    xz_vertices.push_back((float)i);
    xz_vertices.push_back(gridHalfSize);
    xz_vertices.push_back(0.0f);
    xz_vertices.push_back((float)i);
  }
  glGenVertexArrays(1, &xzPlaneVAO);
  glGenBuffers(1, &xzPlaneVBO);
  glBindVertexArray(xzPlaneVAO);
  glBindBuffer(GL_ARRAY_BUFFER, xzPlaneVBO);
  glBufferData(GL_ARRAY_BUFFER, xz_vertices.size() * sizeof(float),
               &xz_vertices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  // --- XY Plane (Z=0, Blue) ---
  std::vector<float> xy_vertices;
  for (int i = -gridSize; i <= gridSize; ++i) {
    xy_vertices.push_back((float)i);
    xy_vertices.push_back(-gridHalfSize);
    xy_vertices.push_back(0.0f);
    xy_vertices.push_back((float)i);
    xy_vertices.push_back(gridHalfSize);
    xy_vertices.push_back(0.0f);
    xy_vertices.push_back(-gridHalfSize);
    xy_vertices.push_back((float)i);
    xy_vertices.push_back(0.0f);
    xy_vertices.push_back(gridHalfSize);
    xy_vertices.push_back((float)i);
    xy_vertices.push_back(0.0f);
  }
  glGenVertexArrays(1, &xyPlaneVAO);
  glGenBuffers(1, &xyPlaneVBO);
  glBindVertexArray(xyPlaneVAO);
  glBindBuffer(GL_ARRAY_BUFFER, xyPlaneVBO);
  glBufferData(GL_ARRAY_BUFFER, xy_vertices.size() * sizeof(float),
               &xy_vertices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  // --- YZ Plane (X=0, Red) ---
  std::vector<float> yz_vertices;
  for (int i = -gridSize; i <= gridSize; ++i) {
    yz_vertices.push_back(0.0f);
    yz_vertices.push_back((float)i);
    yz_vertices.push_back(-gridHalfSize);
    yz_vertices.push_back(0.0f);
    yz_vertices.push_back((float)i);
    yz_vertices.push_back(gridHalfSize);
    yz_vertices.push_back(0.0f);
    yz_vertices.push_back(-gridHalfSize);
    yz_vertices.push_back((float)i);
    yz_vertices.push_back(0.0f);
    yz_vertices.push_back(gridHalfSize);
    yz_vertices.push_back((float)i);
  }
  glGenVertexArrays(1, &yzPlaneVAO);
  glGenBuffers(1, &yzPlaneVBO);
  glBindVertexArray(yzPlaneVAO);
  glBindBuffer(GL_ARRAY_BUFFER, yzPlaneVBO);
  glBufferData(GL_ARRAY_BUFFER, yz_vertices.size() * sizeof(float),
               &yz_vertices[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  glBindVertexArray(0); // Unbind

  // --- Instance Data Setup ---
  srand(
      static_cast<unsigned int>(glfwGetTime())); // seed random number generator
  cubes.reserve(NUM_CUBES);
  for (int i = 0; i < NUM_CUBES; ++i) {
    glm::mat4 model = glm::mat4(1.0f);
    float x = (static_cast<float>(rand() % 20000) / 100.0f) - 100.0f;
    float y = (static_cast<float>(rand() % 5000) / 100.0f);
    float z = (static_cast<float>(rand() % 20000) / 100.0f) - 100.0f;
    model = glm::translate(model, glm::vec3(x, y, z));
    float angle = static_cast<float>(rand() % 360);
    model = glm::rotate(model, glm::radians(angle),
                        glm::normalize(glm::vec3(0.4f, 0.6f, 0.8f)));
    float scale = (static_cast<float>(rand()) / RAND_MAX) * 0.8f + 0.2f;
    model = glm::scale(model, glm::vec3(scale));

    CubeInstance c;
    c.modelMatrix = model;
    cubes.push_back(c);
  }

  // --- Instance Buffer Setup (for Instanced Rendering) ---
  unsigned int instanceVBO;
  glGenBuffers(1, &instanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, NUM_CUBES * sizeof(glm::mat4),
               &cubes[0].modelMatrix, GL_DYNAMIC_DRAW);

  glBindVertexArray(cubeVAO);
  // Per-instance Model Matrix attribute
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)0);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(sizeof(glm::vec4)));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(2 * sizeof(glm::vec4)));
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(3 * sizeof(glm::vec4)));
  glVertexAttribDivisor(2, 1);
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);

  // --- Indirect Draw Setup ---
  struct DrawCommand {
    GLuint count;
    GLuint instanceCount;
    GLuint first;
    GLuint baseInstance;
  };
  DrawCommand drawCmd;
  drawCmd.count = 36;
  drawCmd.instanceCount = NUM_CUBES;
  drawCmd.first = 0;
  drawCmd.baseInstance = 0;

  unsigned int indirectBuffer;
  glGenBuffers(1, &indirectBuffer);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
  glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawCommand), &drawCmd,
               GL_STATIC_DRAW);

  // SSBO for indirect drawing model matrices
  unsigned int ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_CUBES * sizeof(CubeInstance),
               &cubes[0], GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                   ssbo); // bind to binding point 0

  glBindVertexArray(0);

  // --- Performance Query ---
  GLuint queryID[2];
  glGenQueries(2, queryID);
  GLuint64 gpuTime = 0;

  // --- RENDER LOOP ---
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    // --- Start ImGui Frame ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- UI Window ---
    {
      ImGui::Begin("Performance Analyzer");

      float fps = ImGui::GetIO().Framerate;
      float cpu_ms = deltaTime * 1000.0f;
      float gpu_ms = gpuTime / 1000000.0;

      ImGui::Text("FPS: %.1f", fps);
      ImGui::Text("CPU Time: %.3f ms", cpu_ms);
      ImGui::Text("GPU Time: %.3f ms", gpu_ms);
      ImGui::Separator();

      ImGui::Text("Number of Cubes: %d", NUM_CUBES);
      ImGui::Separator();

      ImGui::Text("Rendering Method:");
      ImGui::RadioButton("Uniform", (int *)&currentMethod, UNIFORM);
      ImGui::RadioButton("Instanced", (int *)&currentMethod, INSTANCED);
      ImGui::RadioButton("Indirect", (int *)&currentMethod, INDIRECT);
      ImGui::Separator();

      ImGui::Text("Controls:");
      ImGui::Text("W/A/S/D/Q/E: Move Camera");
      ImGui::Text("Mouse: Look Around");
      ImGui::Text("F1: Toggle UI Mouse Capture");

      ImGui::End();
    }

    // --- Update Cube Rotations ---
    for (auto &cube : cubes) {
      cube.modelMatrix =
          glm::rotate(cube.modelMatrix, deltaTime * 0.2f,
                      glm::normalize(glm::vec3(0.5f, 1.0f, 0.0f)));
    }

    // --- Rendering ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // --- Draw Grid ---
    // --- NEW CODE ---
    // --- Draw 3D Planes ---
    gridShader.use();
    gridShader.setMat4("projection", projection);
    gridShader.setMat4("view", view);

    // Draw XZ Plane (Green)
    gridShader.setVec3("planeColor", glm::vec3(0.0f, 0.4f, 0.0f));
    glBindVertexArray(xzPlaneVAO);
    glDrawArrays(GL_LINES, 0, xz_vertices.size() / 3);

    // Draw XY Plane (Blue)
    gridShader.setVec3("planeColor", glm::vec3(0.0f, 0.0f, 0.4f));
    glBindVertexArray(xyPlaneVAO);
    glDrawArrays(GL_LINES, 0, xy_vertices.size() / 3);

    // Draw YZ Plane (Red)
    gridShader.setVec3("planeColor", glm::vec3(0.4f, 0.0f, 0.0f));
    glBindVertexArray(yzPlaneVAO);
    glDrawArrays(GL_LINES, 0, yz_vertices.size() / 3);
    // --- Begin GPU Timer ---
    glBeginQuery(GL_TIME_ELAPSED, queryID[0]);

    // --- Draw Cubes ---
    glBindVertexArray(cubeVAO);

    if (currentMethod == UNIFORM) {
      uniformShader.use();
      uniformShader.setMat4("projection", projection);
      uniformShader.setMat4("view", view);
      // This is intentionally inefficient for demonstration
      for (const auto &cube : cubes) {
        uniformShader.setMat4("model", cube.modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 36);
      }
    } else if (currentMethod == INSTANCED) {
      // Update instance VBO with new model matrices
      glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, cubes.size() * sizeof(CubeInstance),
                      &cubes[0]);

      instancedShader.use();
      instancedShader.setMat4("projection", projection);
      instancedShader.setMat4("view", view);
      instancedShader.setBool("useSSBO", false);
      glDrawArraysInstanced(GL_TRIANGLES, 0, 36, cubes.size());
    } else if (currentMethod == INDIRECT) {
      // Update SSBO with new model matrices
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                      cubes.size() * sizeof(CubeInstance), &cubes[0]);

      instancedShader.use(); // Reuse the instanced shader
      instancedShader.setMat4("projection", projection);
      instancedShader.setMat4("view", view);
      instancedShader.setBool("useSSBO", true);
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
      glDrawArraysIndirect(GL_TRIANGLES, (void *)0);
    }

    // --- End GPU Timer ---
    glEndQuery(GL_TIME_ELAPSED);

    // --- Render ImGui ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // --- Finalize Frame ---
    glGetQueryObjectui64v(queryID[1], GL_QUERY_RESULT, &gpuTime);
    std::swap(queryID[0], queryID[1]);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // --- Cleanup ---
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteVertexArrays(1, &xzPlaneVAO);
  glDeleteVertexArrays(1, &xyPlaneVAO);
  glDeleteVertexArrays(1, &yzPlaneVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteBuffers(1, &xzPlaneVBO);
  glDeleteBuffers(1, &xyPlaneVBO);
  glDeleteBuffers(1, &yzPlaneVBO);
  glDeleteBuffers(1, &instanceVBO);
  glDeleteBuffers(1, &indirectBuffer);
  glDeleteBuffers(1, &ssbo);
  glDeleteQueries(2, queryID);

  glfwTerminate();
  return 0;
}

// --- Input Processing and Callbacks ---
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    camera.ProcessKeyboard(UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    camera.ProcessKeyboard(DOWN, deltaTime);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_F1) {
      ui_capture_mouse = !ui_capture_mouse;
      if (ui_capture_mouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true; // Prevent camera jump
      }
    }
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  if (ui_capture_mouse) {
    return;
  }

  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;
  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  if (ImGui::GetIO().WantCaptureMouse) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    return;
  }
  camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
