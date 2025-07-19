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
const unsigned int NUM_CUBES =
    50000; // Set a high number to see the performance impact

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadShader(const char *vertexSrc, const char *fragmentSrc);

// --- Global Variables ---
// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 155.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Rendering mode
enum RenderMethod { UNIFORM = 1, INSTANCED = 2, INDIRECT = 3 };
RenderMethod currentMethod = INSTANCED; // Start with the efficient method

// --- Shader Sources ---

// Vertex Shader for the Uniform method
const char *vs_uniform = R"glsl(
    #version 430 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Vertex Shader for Instanced and Indirect methods
const char *vs_instanced = R"glsl(
    #version 430 core
    layout (location = 0) in vec3 aPos;
    // The model matrix is passed as an instanced vertex attribute
    layout (location = 1) in mat4 aInstanceMatrix;

    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * aInstanceMatrix * vec4(aPos, 1.0);
    }
)glsl";

// A simple Fragment Shader for all methods
const char *fs_simple = R"glsl(
    #version 430 core
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 0.8, 0.2, 1.0); // Orange color
    }
)glsl";

// For Indirect drawing, we need this struct to match the layout expected by
// glDrawArraysIndirect
struct DrawArraysIndirectCommand {
  GLuint count;
  GLuint instanceCount;
  GLuint first;
  GLuint baseInstance;
};

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

  // --- Shader Compilation ---
  unsigned int uniformShader = loadShader(vs_uniform, fs_simple);
  unsigned int instancedShader = loadShader(vs_instanced, fs_simple);

  // --- Cube Vertex Data ---
  float vertices[] = {
      // back face
      -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
      -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
      // front face
      -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
      // left face
      -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
      // right face
      0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
      -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
      // bottom face
      -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
      0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f,
      // top face
      -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f};
  unsigned int cubeVAO, cubeVBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  // --- Instancing Setup ---
  // Generate a buffer for the model matrices
  std::vector<glm::mat4> modelMatrices(NUM_CUBES);
  float radius = 100.0f;
  float offset = 25.0f;
  for (unsigned int i = 0; i < NUM_CUBES; i++) {
    glm::mat4 model = glm::mat4(1.0f);
    // 1. translation: displace along circle with 'radius' in range [-offset,
    // offset]
    float angle = (float)i / (float)NUM_CUBES * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x = sin(angle) * radius + displacement;
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement *
              0.4f; // keep height of field smaller compared to width of x and z
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z = cos(angle) * radius + displacement;
    model = glm::translate(model, glm::vec3(x, y, z));

    // 2. scale: Scale between 0.05 and 0.25f
    float scale = (rand() % 20) / 100.0f + 0.05f;
    model = glm::scale(model, glm::vec3(scale));

    // 3. rotation: add random rotation around a (semi)randomly picked rotation
    // axis vector
    float rotAngle = (rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    modelMatrices[i] = model;
  }

  // Create and configure the VBO for instance data
  unsigned int instanceVBO;
  glGenBuffers(1, &instanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, NUM_CUBES * sizeof(glm::mat4),
               &modelMatrices[0], GL_DYNAMIC_DRAW);

  // Set up vertex attribute pointers for the instance matrix.
  // A mat4 is equivalent to 4 vec4s.
  glBindVertexArray(cubeVAO);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)0);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(sizeof(glm::vec4)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(2 * sizeof(glm::vec4)));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (void *)(3 * sizeof(glm::vec4)));

  // Tell OpenGL this is an instanced vertex attribute.
  // This advances the attribute once per instance, not per vertex.
  glVertexAttribDivisor(1, 1);
  glVertexAttribDivisor(2, 1);
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);

  glBindVertexArray(0);

  // --- Indirect Drawing Setup ---
  DrawArraysIndirectCommand command;
  command.count = 36; // 36 vertices per cube
  command.instanceCount = NUM_CUBES;
  command.first = 0;
  command.baseInstance = 0;

  unsigned int indirectBuffer;
  glGenBuffers(1, &indirectBuffer);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
  glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(command), &command,
               GL_STATIC_DRAW);

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
      std::string method_str;
      switch (currentMethod) {
      case UNIFORM:
        method_str = "Uniform";
        break;
      case INSTANCED:
        method_str = "Instanced";
        break;
      case INDIRECT:
        method_str = "Indirect";
        break;
      }
      std::string title =
          "OpenGL Instancing Benchmark | Method: " + method_str + " | " +
          std::to_string(frameCount) + " FPS" + " | " +
          std::to_string(1000.0 / frameCount) + " ms/frame";
      glfwSetWindowTitle(window, title.c_str());

      frameCount = 0;
      previousTime = currentFrame;
    }

    // --- Input ---
    processInput(window);

    // --- Rendering ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Update cube rotations ---
    // This is a CPU-intensive task that we do every frame for all methods
    // to simulate a dynamic scene.
    float rotationSpeed = (float)glfwGetTime() * 0.2f;
    for (unsigned int i = 0; i < NUM_CUBES; i++) {
      modelMatrices[i] =
          glm::rotate(modelMatrices[i], rotationSpeed * deltaTime,
                      glm::vec3(0.5f, 1.0f, 0.0f));
    }
    // For instanced/indirect, we update the entire buffer at once
    if (currentMethod == INSTANCED || currentMethod == INDIRECT) {
      glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_CUBES * sizeof(glm::mat4),
                      &modelMatrices[0]);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // --- View/Projection matrices ---
    glm::mat4 projection =
        glm::perspective(glm::radians(45.0f),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // --- Draw based on selected method ---
    switch (currentMethod) {
    case UNIFORM:
      glUseProgram(uniformShader);
      glUniformMatrix4fv(glGetUniformLocation(uniformShader, "view"), 1,
                         GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(uniformShader, "projection"), 1,
                         GL_FALSE, glm::value_ptr(projection));
      glBindVertexArray(cubeVAO);
      for (unsigned int i = 0; i < NUM_CUBES; i++) {
        glUniformMatrix4fv(glGetUniformLocation(uniformShader, "model"), 1,
                           GL_FALSE, glm::value_ptr(modelMatrices[i]));
        glDrawArrays(GL_TRIANGLES, 0, 36);
      }
      glBindVertexArray(0);
      break;

    case INSTANCED:
      glUseProgram(instancedShader);
      glUniformMatrix4fv(glGetUniformLocation(instancedShader, "view"), 1,
                         GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(instancedShader, "projection"), 1,
                         GL_FALSE, glm::value_ptr(projection));
      glBindVertexArray(cubeVAO);
      glDrawArraysInstanced(GL_TRIANGLES, 0, 36, NUM_CUBES);
      glBindVertexArray(0);
      break;

    case INDIRECT:
      glUseProgram(instancedShader); // Uses the same shader as instanced
      glUniformMatrix4fv(glGetUniformLocation(instancedShader, "view"), 1,
                         GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(instancedShader, "projection"), 1,
                         GL_FALSE, glm::value_ptr(projection));
      glBindVertexArray(cubeVAO);
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
      // The parameters for the draw call are read from the bound indirect
      // buffer
      glDrawArraysIndirect(GL_TRIANGLES, (void *)0);
      glBindVertexArray(0);
      break;
    }

    // --- Swap buffers and poll events ---
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // --- Cleanup ---
  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteBuffers(1, &instanceVBO);
  glDeleteBuffers(1, &indirectBuffer);
  glDeleteProgram(uniformShader);
  glDeleteProgram(instancedShader);

  glfwTerminate();
  return 0;
}

// --- Helper Functions ---

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    currentMethod = UNIFORM;
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    currentMethod = INSTANCED;
  if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
    currentMethod = INDIRECT;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

unsigned int loadShader(const char *vertexSrc, const char *fragmentSrc) {
  unsigned int vertexShader, fragmentShader, shaderProgram;
  int success;
  char infoLog[512];

  // Vertex Shader
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSrc, NULL);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  // Fragment Shader
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSrc, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }

  // Shader Program
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
              << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shaderProgram;
}
