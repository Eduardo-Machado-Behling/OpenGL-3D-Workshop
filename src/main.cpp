#include <cstddef>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <type_traits>

#include "utils.hpp"

// --- Configuration ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam);

// --- Global Variables ---
// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Shader Code
std::string vertexShaderCode = ReadFile("../assets/shaders/vertex.vert");
const char* vertexShaderSource = vertexShaderCode.c_str();

std::string fragmentShaderCode = ReadFile("../assets/shaders/texture.frag");
const char* fragmentShaderSource = fragmentShaderCode.c_str();

int main() {
  // --- GLFW and GLAD Initialization ---
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Texture Checkpoint", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  glfwSwapInterval(0); // disable v-sync

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // --- ImGui config ---
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(
      main_scale); // Bake a fixed style scale. (until we have a solution for
                  // dynamic style scaling, changing this requires resetting
                  // Style + calling this again)
  style.FontScaleDpi =
      main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true
                  // makes this unnecessary. We leave both here for
                  // documentation purpose)

  // --- OpenGL Information ---
  const GLubyte *vendor = glGetString(GL_VENDOR);
  const GLubyte *renderer = glGetString(GL_RENDERER);
  const GLubyte *version = glGetString(GL_VERSION);
  const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

  std::cout << "OpenGL Vendor: " << vendor << std::endl;
  std::cout << "OpenGL Renderer: " << renderer << std::endl;
  std::cout << "OpenGL Version: " << version << std::endl;
  std::cout << "GLSL Version: " << glslVersion << std::endl;

  GLint flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Makes sure errors are displayed synchronously
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");

  // --- Buffers (VAO) ---
  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;

    Vertex(glm::vec3 pos, glm::vec2 uv) : pos(pos), uv(uv) {}
  } vertices[] = {
    {{-0.5, -0.5, 0.0}, {0.0, 0.0}}, // 0
    {{-0.5, 0.5, 0.0}, {0.0, 1.0}},  // 1
    {{0.5, 0.5, 0.0}, {1.0, 1.0}},   // 2
    {{0.5, -0.5, 0.0}, {1.0, 0.0}},  // 3
  };

  GLuint index_array[] = {
    0, 1, 2,
    0, 3, 2,
  };

  unsigned int element_count = sizeof(index_array) / sizeof(GLuint);

  // --- Buffers (VBO) ---
  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
      (void *)offsetof(std::remove_reference_t<decltype(vertices[0])>, pos));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
      1, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]),
      (void *)offsetof(std::remove_reference_t<decltype(vertices[0])>, uv));
  glEnableVertexAttribArray(1);

  // --- Buffers (EBO) ---
  GLuint EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); 
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_array), &index_array, GL_STATIC_DRAW);

  // --- Shader Compilation ---
  // compile vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // check for vertex shader compile errors
  int success;
  char info_log[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  // compile frag shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // check for fragment shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, info_log);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  //  --- Program ---
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  // --- Texture load ---
  GLuint texture_id = LoadTexture("../assets/textures/test.png");

  // --- Shader Cleanup ---
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // --- OpenGL Global State ---
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_PROGRAM_POINT_SIZE);

  // --- Render Loop ---
  int frameCount = 0;
  double previousTime = glfwGetTime();

  GLuint tex_coord_desloc_loc = glGetUniformLocation(program, "uv_desloc");
  glm::vec2 tex_coord_desloc(0.0, 0.0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  glUseProgram(program);
  
  glUniform1i(glGetUniformLocation(program, "texture1"), texture_id);

  while (!glfwWindowShouldClose(window)) {
    // --- Per-frame time logic ---
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // --- Performance Measurement ---
    frameCount++;
    if (currentFrame - previousTime >= 1.0) {
      std::string method_str;
      std::string title = "OpenGL Window | " + std::to_string(frameCount) + " FPS" + " | " + std::to_string(1000.0 / frameCount) + " ms/frame";
      glfwSetWindowTitle(window, title.c_str());

      frameCount = 0;
      previousTime = currentFrame;
    }

    // --- Input ---
    processInput(window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Variables Panel");

      ImGui::DragFloat2("Desloc UV", &tex_coord_desloc[0], 0.005, -1.0f, 1.0f);

      ImGui::Text("Horizontal");
      if (ImGui::Button("GL_REPEATE S")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      if (ImGui::Button("GL_MIRRORED_REPEAT S")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); 
      if (ImGui::Button("GL_CLAMP_TO_EDGE S")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 

      ImGui::Text("Vertical");
      if (ImGui::Button("GL_REPEATE T")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      if (ImGui::Button("GL_MIRRORED_REPEAT T")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); 
      if (ImGui::Button("GL_CLAMP_TO_EDGE T")) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

      ImGui::End();
    }

    ImGui::Render();

    // --- Rendering ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(VAO);

    glUniform2fv(tex_coord_desloc_loc, 1, glm::value_ptr(tex_coord_desloc));

    glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // --- Swap buffers and poll events ---
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteProgram(program);
  glDeleteVertexArrays(1, &VAO);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
  // Ignore non-significant error/warning codes
  // You can customize this to filter out messages you don't care about
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    return;

  std::cout << "---------------" << std::endl;
  std::cout << "Debug message (" << id << "): " << message << std::endl;

  switch (source) {
  case GL_DEBUG_SOURCE_API:
    std::cout << "Source: API";
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    std::cout << "Source: Window System";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    std::cout << "Source: Shader Compiler";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    std::cout << "Source: Third Party";
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    std::cout << "Source: Application";
    break;
  case GL_DEBUG_SOURCE_OTHER:
    std::cout << "Source: Other";
    break;
  }
  std::cout << std::endl;

  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    std::cout << "Type: Error";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    std::cout << "Type: Deprecated Behaviour";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    std::cout << "Type: Undefined Behaviour";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    std::cout << "Type: Portability";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    std::cout << "Type: Performance";
    break;
  case GL_DEBUG_TYPE_MARKER:
    std::cout << "Type: Marker";
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    std::cout << "Type: Push Group";
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    std::cout << "Type: Pop Group";
    break;
  case GL_DEBUG_TYPE_OTHER:
    std::cout << "Type: Other";
    break;
  }
  std::cout << std::endl;

  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    std::cout << "Severity: high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    std::cout << "Severity: medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    std::cout << "Severity: low";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    std::cout << "Severity: notification";
    break;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}
