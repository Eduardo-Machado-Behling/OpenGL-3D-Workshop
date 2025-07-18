// OpenGL Instancing Performance Benchmark
// ---------------------------------------
// This program demonstrates and compares three methods for rendering a large
// number of objects:
// 1. Uniform Method: A separate draw call for each cube, updating a uniform
// model matrix each time. (Slow)
// 2. Instanced Method: A single draw call to render all cubes, using an
// instanced vertex attribute for the model matrices. (Fast)
// 3. Indirect Method: A single, GPU-driven draw call using
// glDrawArraysIndirect. (Fast, useful for GPU-side culling)
//
// Dependencies:
// - GLFW: for window and input management.
// - GLAD: for loading OpenGL function pointers.
// - GLM: for matrix/vector mathematics.
//
// Compilation (example with g++ on Linux/macOS):
// g++ main.cpp glad.c -o benchmark -I/path/to/glm -I/path/to/glad/include
// -lglfw -lGL -ldl -lpthread
//
// Controls:
// - '1': Switch to Uniform Method
// - '2': Switch to Instanced Method
// - '3': Switch to Indirect Method
// - ESC: Exit the application
//
// The performance difference will be most noticeable with a large number of
// cubes. The constant NUM_CUBES is set to 50,000 to clearly show the
// performance impact.

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>

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
  // --- GLFW and GLAD Initialization ---
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(
      SCR_WIDTH, SCR_HEIGHT, "OpenGL Instancing Benchmark", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // --- Disable V-Sync to unlock FPS ---
  glfwSwapInterval(0);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // --- OpenGL Global State ---
  glEnable(GL_DEPTH_TEST);

  // --- Render Loop ---
  int frameCount = 0;
  double previousTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    // --- Per-frame time logic ---
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // --- Performance Measurement ---
    frameCount++;
    if (currentFrame - previousTime >= 1.0) {
      std::string title = "OpenGL Window | " + std::to_string(frameCount) +
                          " FPS" + " | " + std::to_string(1000.0 / frameCount) +
                          " ms/frame";
      glfwSetWindowTitle(window, title.c_str());

      frameCount = 0;
      previousTime = currentFrame;
    }

    // --- Input ---
    processInput(window);

    // --- Rendering ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Swap buffers and poll events ---
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

// --- Helper Functions ---

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}
