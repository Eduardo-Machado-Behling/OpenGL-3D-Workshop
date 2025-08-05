#include "mesh.hpp"
#include "model.hpp"
#include <cstddef>
#include <filesystem>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/io.hpp>

#include <iostream>
#include <string>
#include <type_traits>

#include "camera.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui_internal.h"

#include "shader.hpp"
#include "window.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <vector>

// --- Configuration ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

// --- Global Variables ---
// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fov = 45;
bool captureMouse = false;

float aspect = (float)SCR_WIDTH / SCR_HEIGHT;

CameraPerpective camera;
bool keys[1024] = {false};

int main() {
  Window window(SCR_WIDTH, SCR_HEIGHT);

  window.set(glfwSetFramebufferSizeCallback, framebuffer_size_callback);
  window.set(glfwSetKeyCallback, key_callback);
  // window.set(glfwSetCursorPosCallback, mouse_callback);
  // window.set(glfwSetScrollCallback, scroll_callback);

  Model modell = Model::Load("./bin/assets/models/space craft.blend");
  Shader::Program program;
  Model ground = Model::Load("./bin/assets/models/01_Model_2_20w__.obj");

  program.attachShader("default.vert").attachShader("default.frag").link();
  glm::vec3 pos(100, 100, 0);
  glm::vec3 scale(0.1);
  glm::vec3 rot(-90, 0, 0);

  glm::vec2 z(0.1, 1'000);

  // --- OpenGL Global State ---
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_STENCIL_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_PROGRAM_POINT_SIZE);

  // --- Render Loop ---
  int frameCount = 0;
  double previousTime = glfwGetTime();
  float vel = 100;

  glm::vec3 _pos = glm::vec3(0, -50, 0);
  glm::vec3 _scale = glm::vec3(100, 100, 1);
  glm::vec3 _rot = glm::vec3(-90, 0, 0);

  camera.pos(camera.pos() + glm::vec3(0, -23.621, 0));

  while (!window.shouldClose()) {
    // --- Per-frame time logic ---
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // --- Performance Measurement ---
    frameCount++;
    if (currentFrame - previousTime >= 1.0) {
      std::string method_str;
      std::string title = "OpenGL Window | " + std::to_string(frameCount) +
                          " FPS" + " | " + std::to_string(1000.0 / frameCount) +
                          " ms/frame";
      // glfwSetWindowTitle(window, title.c_str());
      window.setTitle(title.c_str());

      frameCount = 0;
      previousTime = currentFrame;
    }

    glm::vec3 velocity(0.0f);
    if (keys[GLFW_KEY_W])
      velocity -= camera.front();
    if (keys[GLFW_KEY_S])
      velocity += camera.front();
    if (keys[GLFW_KEY_A])
      velocity += glm::normalize(glm::cross(camera.front(), camera.up()));
    if (keys[GLFW_KEY_D])
      velocity -= glm::normalize(glm::cross(camera.front(), camera.up()));
    if (keys[GLFW_KEY_SPACE])
      velocity += camera.up();
    if (keys[GLFW_KEY_LEFT_CONTROL])
      velocity -= camera.up();

    glm::vec3 newPos = camera.pos() + velocity * deltaTime * vel;
    newPos.y = newPos.y < -25 ? -25 : newPos.y;
    camera.pos(newPos);

    pos = camera.pos() + (glm::vec3(0, -1, 1) * 20.f * scale.x);

    camera.lookAt(pos);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Variables Panel");

      const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_RowBg |
                                    ImGuiTableFlags_Resizable;

      if (ImGui::BeginTable("camera", 4, flags)) {
        ImGui::TableSetupColumn("Position (X,Y,Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Up (X, Y, Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Front (X, Y, Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Vel", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGui::PushID(0); // Ensure unique widget IDs for each row

        // Column 1: Index
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##pos", &camera.pos()[0]);

        // Column 2: Position Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##scale", &camera.up()[0]);

        // Column 3: Color Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##rot", &camera.front()[0]);

        ImGui::TableNextColumn();
        ImGui::DragFloat("##vel", &vel);

        ImGui::PopID(); // Don't forget to pop the ID
      }
      ImGui::EndTable();

      if (ImGui::BeginTable("model", 3, flags)) {
        ImGui::TableSetupColumn("Position (X,Y,Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Scale (X, Y, Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Rotate (X, Y, Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGui::PushID(0); // Ensure unique widget IDs for each row

        // Column 1: Index
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##pos", &pos[0]);

        // Column 2: Position Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##scale", &scale[0]);

        // Column 3: Color Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##rot", &rot[0]);

        ImGui::PopID();   // Don't forget to pop the ID
                          //
        ImGui::PushID(1); // Ensure unique widget IDs for each row

        // Column 1: Index
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##pos", &_pos[0]);

        // Column 2: Position Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##scale", &_scale[0]);

        // Column 3: Color Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat3("##rot", &_rot[0]);

        ImGui::PopID(); // Don't forget to pop the ID
      }
      ImGui::EndTable();

      if (ImGui::BeginTable("view", 3, flags)) {
        ImGui::TableSetupColumn("fov", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("aspect", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Window (Zmin, Zmax)",
                                ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableHeadersRow();
        ImGui::PushID(0); // Ensure unique widget IDs for each row
                          //
        // Column 1: Index
        ImGui::TableNextColumn();
        ImGui::DragFloat("##x", &fov);

        // Column 2: Position Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat("##y", &aspect);

        // Column 3: Color Editor
        ImGui::TableNextColumn();
        ImGui::DragFloat2("##z", &z[0]);

        ImGui::PopID(); // Don't forget to pop the ID
      }
      ImGui::EndTable();

      ImGui::End();
    }

    ImGui::Render();

    // --- Rendering ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.bind();
    camera.setProj(fov, aspect, z[0], z[1]);
    glm::mat4 proj = camera.getProj();
    glUniformMatrix4fv(program.getLocation("proj"), 1, GL_FALSE,
                       glm::value_ptr(proj));

    glm::mat4 view = camera.getView();
    glUniformMatrix4fv(program.getLocation("view"), 1, GL_FALSE,
                       glm::value_ptr(view));

    // --- Build the Model Matrix Correctly ---
    // 1. Start with the identity matrix
    glm::mat4 model = glm::mat4(1.0f);
    // 2. Apply transformations in reverse order: scale, then rotate, then
    // translate
    model = glm::translate(model, pos);
    model =
        glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model =
        glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model =
        glm::rotate(model, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);

    glUniformMatrix4fv(program.getLocation("model"), 1, GL_FALSE,
                       glm::value_ptr(model));

    glUniform1i(program.getLocation("type"), 0);
    modell.Draw(program);

    model = glm::mat4(1);
    model = glm::translate(model, _pos);

    model =
        glm::rotate(model, glm::radians(_rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model =
        glm::rotate(model, glm::radians(_rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model =
        glm::rotate(model, glm::radians(_rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, _scale);
    glUniformMatrix4fv(program.getLocation("model"), 1, GL_FALSE,
                       glm::value_ptr(model));

    glUniform1i(program.getLocation("type"), 0);
    ground.Draw(program);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swapBuffers();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  aspect = (float)width / height;
}

// In your key callback:
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
  if (key >= 0 && key < 1024) {
    if (action == GLFW_PRESS)
      keys[key] = true;
    else if (action == GLFW_RELEASE)
      keys[key] = false;
  }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  return;
  static float lastX = 400, lastY = 300;
  static bool firstMouse = true;
  ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
  if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) != GLFW_RELEASE) {
    firstMouse = true;
    return;
  }

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // reversed since y-coordinates go bottom to top
  lastX = xpos;
  lastY = ypos;

  float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  glm::vec2 rot = camera.rot();
  rot.x += xoffset;
  rot.y += yoffset;

  // Clamp pitch
  if (rot.y > 89.0f)
    rot.y = 89.0f;
  if (rot.y < -89.0f)
    rot.y = -89.0f;

  camera.rot(rot);

  glm::vec3 direction;
  direction.x = cos(glm::radians(rot.x)) * cos(glm::radians(rot.y));
  direction.y = sin(glm::radians(rot.y));
  direction.z = sin(glm::radians(rot.x)) * cos(glm::radians(rot.y));
  camera.front(glm::normalize(direction));
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
  fov -= (float)yoffset;
  if (fov < 1.0f)
    fov = 1.0f;
  if (fov > 45.0f)
    fov = 45.0f;
}
