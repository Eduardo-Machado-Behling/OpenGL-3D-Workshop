#include <cstddef>
#include <cstdint>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <type_traits>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui_internal.h"

#include "shader.hpp"
#include "window.hpp"

// --- Configuration ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

// --- Global Variables ---
// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {
  Window window(SCR_WIDTH, SCR_HEIGHT);

  window.set(glfwSetFramebufferSizeCallback, framebuffer_size_callback);

  // --- Buffers (VAO) ---
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    Vertex(glm::vec3 pos, glm::vec3 color) : pos(pos), color(color) {}
  } vertices[] = {
      {{0.0, 0.0, 0.5}, {0.0, 1.0, 1.0}},
      {{0.0, 1.0, 0.5}, {1.0, 0.0, 0.0}},
      {{1.0, 0.0, 0.5}, {0.0, 1.0, 0.0}},
      {{1.0, 1.0, 0.5}, {0.0, 0.0, 1.0}},
  };
  uint32_t indices[] = {0, 1, 2, 1, 2, 3};

  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0],
               GL_DYNAMIC_DRAW);

  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
      (void *)offsetof(std::remove_reference_t<decltype(vertices[0])>, pos));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
      (void *)offsetof(std::remove_reference_t<decltype(vertices[0])>, color));
  glEnableVertexAttribArray(1);

  GLuint EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0],
               GL_STATIC_DRAW);

  Shader::Program program;

  program.attachShader("default.vert").attachShader("default.frag").link();
  glm::vec3 pos(0);
  glm::vec3 scale(1);
  glm::vec3 rot(0);

  glm::vec<2, int32_t> x(0, SCR_WIDTH);
  glm::vec<2, int32_t> y(0, SCR_HEIGHT);
  glm::vec<2, int32_t> z(-1, 1);

  // --- OpenGL Global State ---
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_PROGRAM_POINT_SIZE);

  // --- Render Loop ---
  int frameCount = 0;
  double previousTime = glfwGetTime();

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
      if (ImGui::BeginTable("vertex_editor", 3, flags)) {
        ImGui::TableSetupColumn("Vertex #", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Position (X, Y, Z)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // 2. Loop through your vertices and create a row for each
        for (size_t i = 0; i < sizeof(vertices) / sizeof(*vertices);
             i++) // Replace with your vertex count
        {
          ImGui::PushID(i); // Ensure unique widget IDs for each row

          // Column 1: Index
          ImGui::TableNextColumn();
          ImGui::Text("%zu", i);

          // Column 2: Position Editor
          ImGui::TableNextColumn();
          ImGui::DragFloat3("##pos", &vertices[i].pos[0], 0.005, -1.5f, 1.5f);

          // Column 3: Color Editor
          ImGui::TableNextColumn();
          ImGui::ColorEdit3("##color", &vertices[i].color[0]);

          ImGui::PopID(); // Don't forget to pop the ID
        }
        ImGui::EndTable();
      }

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

        ImGui::PopID(); // Don't forget to pop the ID
      }
      ImGui::EndTable();

      if (ImGui::BeginTable("view", 3, flags)) {
        ImGui::TableSetupColumn("Window (Xmin, Xmax)",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Window (Ymin, Ymax)",
                                ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Window (Zmin, Zmax)",
                                ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableHeadersRow();
        ImGui::PushID(0); // Ensure unique widget IDs for each row
                          //
        // Column 1: Index
        ImGui::TableNextColumn();
        ImGui::DragInt2("##x", &x[0]);

        // Column 2: Position Editor
        ImGui::TableNextColumn();
        ImGui::DragInt2("##y", &y[0]);

        // Column 3: Color Editor
        ImGui::TableNextColumn();
        ImGui::DragInt2("##z", &z[0]);

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
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), &vertices[0]);

    glm::mat4 proj = glm::ortho((float)x.x, (float)x.y, (float)y.x, (float)y.y,
                                (float)z.x, (float)z.y);
    glUniformMatrix4fv(program.getLocation("proj"), 1, GL_FALSE,
                       glm::value_ptr(proj));

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

    glDrawArrays(GL_POINTS, 0, 4);
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices),
                   GL_UNSIGNED_INT, NULL);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swapBuffers();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteVertexArrays(1, &VAO);
  return 0;
}

// void processInput(GLFWwindow *window) {
//   if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//     glfwSetWindowShouldClose(window, true);
// }
//
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}
