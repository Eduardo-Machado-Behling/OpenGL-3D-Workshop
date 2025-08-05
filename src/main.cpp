//
// This application demonstrates a highly efficient method for object picking in
// OpenGL. It uses two primary techniques:
// 1. Instanced Rendering: To draw multiple copies of the same object (a cube)
// in a single draw call.
// 2. Multiple Render Targets (MRT): To write to two different textures (one for
// color, one for an object ID)
//    simultaneously within a single rendering pass.
// This avoids separate passes for rendering and picking, significantly
// improving performance.
//

// Third-party library headers
#include <glad/glad.h> // OpenGL function loader. Must be included before GLFW.

#include <GLFW/glfw3.h> // Windowing and input library.
#include <glm/glm.hpp>  // OpenGL mathematics library for vectors and matrices.
#include <glm/gtc/matrix_transform.hpp> // For matrix transformations like translate, rotate, scale.
#include <glm/gtc/type_ptr.hpp> // For converting glm types to pointers for OpenGL.

// Standard C++ library headers
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// --- Forward Declarations ---
// Pre-declaring classes and functions allows us to use them before their full
// definition.
class Shader;
class IdPickingFBO;
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

// --- Global Variables & Constants ---
unsigned int SCR_WIDTH = 1280; // Initial window width.
unsigned int SCR_HEIGHT = 720; // Initial window height.

// Camera vectors defining its position and orientation in world space.
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// A global pointer to our Framebuffer Object, used to access it from the mouse
// callback.
IdPickingFBO *pPickingFBO = nullptr;

// --- Shader Class ---
// A utility class to encapsulate GLSL shader compilation and linking.
class Shader {
public:
  unsigned int ID; // The unique ID of the compiled shader program.

  // Constructor that takes vertex and fragment shader source code as strings.
  Shader(const char *vertexSource, const char *fragmentSource) {
    unsigned int vertex, fragment;

    // Compile Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // Compile Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // Link shaders into a Shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // The individual shader objects are no longer needed after linking.
    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  // Activates the shader program for subsequent rendering calls.
  void use() { glUseProgram(ID); }

  // Utility functions to set uniform values in the shader.
  void setMat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                       &mat[0][0]);
  }
  void setUint(const std::string &name, unsigned int value) const {
    glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setVec3(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
  }

private:
  // A private helper function to check for compilation or linking errors.
  void checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cout
            << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
            << infoLog
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
        std::cout
            << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
            << infoLog
            << "\n -- --------------------------------------------------- -- "
            << std::endl;
      }
    }
  }
};

// --- Framebuffer Object Class for Picking ---
// This class manages the FBO used for Multiple Render Targets (MRT).
class IdPickingFBO {
public:
  IdPickingFBO()
      : m_fbo(0), m_colorTexture(0), m_idTexture(0), m_depthTexture(0) {}

  // Initializes the FBO and all its texture attachments.
  bool Init(unsigned int windowWidth, unsigned int windowHeight) {
    // Clean up old resources before creating new ones (e.g., on window resize).
    if (m_fbo != 0)
      glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTexture != 0)
      glDeleteTextures(1, &m_colorTexture);
    if (m_idTexture != 0)
      glDeleteTextures(1, &m_idTexture);
    if (m_depthTexture != 0)
      glDeleteTextures(1, &m_depthTexture);

    // Create the Framebuffer Object.
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // --- Attachment 0: Color Texture ---
    // This texture stores the final, visible image.
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth, windowHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_colorTexture, 0);

    // --- Attachment 1: ID Texture ---
    // This texture stores a single unsigned integer per pixel for the object
    // ID.
    glGenTextures(1, &m_idTexture);
    glBindTexture(GL_TEXTURE_2D, m_idTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, windowWidth, windowHeight, 0,
                 GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST); // Must be NEAREST for exact ID lookup.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_idTexture, 0);

    // --- Depth Buffer Attachment ---
    // A depth buffer is still required for correct depth testing.
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth,
                 windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_depthTexture, 0);

    // --- Specify Multiple Render Targets ---
    // This crucial step tells OpenGL that our fragment shader will write to two
    // outputs.
    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    // Check if the framebuffer is complete. This is a vital debugging step.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr
          << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status: 0x"
          << std::hex << status << std::endl;
      return false;
    }

    // Unbind the FBO to avoid accidental rendering to it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "MRT FBO initialized/resized successfully." << std::endl;
    return true;
  }

  // Binds this FBO as the target for all subsequent draw calls.
  void BindForWriting() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo); }

  // Binds this FBO for reading, typically before blitting its contents to the
  // screen.
  void BindForBlitting() { glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo); }

  // Reads the object ID from the ID texture at a specific pixel coordinate.
  unsigned int ReadPixel(int x, int y) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);

    // Explicitly set the read source to our ID texture attachment.
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    unsigned int pixelData;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixelData);

    // Reset the read buffer to its default state.
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return pixelData;
  }

  // Destructor to clean up OpenGL resources.
  ~IdPickingFBO() {
    if (m_fbo != 0)
      glDeleteFramebuffers(1, &m_fbo);
    if (m_colorTexture != 0)
      glDeleteTextures(1, &m_colorTexture);
    if (m_idTexture != 0)
      glDeleteTextures(1, &m_idTexture);
    if (m_depthTexture != 0)
      glDeleteTextures(1, &m_depthTexture);
  }

private:
  GLuint m_fbo;
  GLuint m_colorTexture;
  GLuint m_idTexture;
  GLuint m_depthTexture;
};

// --- GLSL Shader Source Code ---

// The vertex shader is responsible for calculating the final screen position of
// each vertex. It also passes per-instance data (color, ID) to the fragment
// shader.
const char *combinedVertexShaderSource = R"glsl(
    #version 330 core
    // Per-vertex attributes
    layout (location = 0) in vec3 aPos;
    // Per-instance attributes
    layout (location = 1) in mat4 instanceModel;
    layout (location = 5) in vec3 instanceColor; // Starts at 5 because mat4 uses locations 1, 2, 3, 4.

    // Data to be passed to the fragment shader
    out vec3 vs_Color;
    flat out uint vs_ObjectID; // 'flat' prevents interpolation of the integer ID.

    // Uniforms (constant for all vertices in a draw call)
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        // Calculate the final clip-space position.
        gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
        // Pass the instance's color through.
        vs_Color = instanceColor;
        // Generate the unique ID from the built-in instance ID.
        // We cast to uint to match the output type and add 1 so IDs start from 1 (0 is background).
        vs_ObjectID = uint(gl_InstanceID) + 1u;
    }
)glsl";

// The fragment shader is responsible for determining the final output for each
// pixel. It writes to two different outputs (render targets) simultaneously.
const char *combinedFragmentShaderSource = R"glsl(
    #version 330 core
    
    // Define two outputs, corresponding to the FBO's color attachments.
    layout (location = 0) out vec4 out_Color; // For the final visual color.
    layout (location = 1) out uvec4 out_ID;   // For the object ID.
    
    // Data received from the vertex shader.
    in vec3 vs_Color;
    flat in uint vs_ObjectID; // 'flat' receives the non-interpolated ID.

    void main() {
        // Write the final color to the first render target.
        out_Color = vec4(vs_Color, 1.0);
        // Write the object ID to the second render target.
        out_ID = uvec4(vs_ObjectID, 0, 0, 0);
    }
)glsl";

// --- Main Application Entry Point ---
int main() {
  // --- Section 1: Initialization ---
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(
      SCR_WIDTH, SCR_HEIGHT, "OpenGL ID Picking (Single Pass MRT)", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // --- Section 2: Shader Compilation ---
  Shader combinedShader(combinedVertexShaderSource,
                        combinedFragmentShaderSource);

  // --- Section 3: Instance Data Setup ---
  const int instanceCount = 3;
  glm::vec3 cubePositions[] = {glm::vec3(-2.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(2.0f, 0.0f, 0.0f)};
  glm::vec3 cubeColors[] = {
      glm::vec3(1.0f, 0.5f, 0.31f), // Coral
      glm::vec3(0.5f, 1.0f, 0.5f),  // Green
      glm::vec3(0.2f, 0.7f, 1.0f)   // Blue
  };
  // Calculate the model matrix for each instance.
  std::vector<glm::mat4> modelMatrices(instanceCount);
  for (int i = 0; i < instanceCount; ++i) {
    modelMatrices[i] = glm::translate(glm::mat4(1.0f), cubePositions[i]);
  }

  // --- Section 4: Vertex Data and VAO Setup ---
  float vertices[] = {
      -0.5f, -0.5f, -0.5f, // back face
      0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,
      -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,

      -0.5f, -0.5f, 0.5f, // front face
      0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
      0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,

      -0.5f, 0.5f,  0.5f, // left face
      -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,

      0.5f,  0.5f,  0.5f, // right face
      0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f,
      -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,

      -0.5f, -0.5f, -0.5f, // bottom face
      0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f,
      0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,

      -0.5f, 0.5f,  -0.5f, // top face
      0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
      0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
  };
  unsigned int cubeVBO, cubeVAO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  // Bind and fill the VBO for the cube's vertex positions.
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  // --- Section 5: Instanced VBO Setup ---
  // Create and fill a VBO for the instance model matrices.
  unsigned int instanceMatrixVBO;
  glGenBuffers(1, &instanceMatrixVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * instanceCount,
               &modelMatrices[0], GL_STATIC_DRAW);
  // A mat4 is treated as 4 vec4s in vertex attributes.
  for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(1 + i);
    glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void *)(sizeof(glm::vec4) * i));
    // This is the key to instancing: the attribute advances once per instance,
    // not per vertex.
    glVertexAttribDivisor(1 + i, 1);
  }
  // Create and fill a VBO for the instance colors.
  unsigned int instanceColorVBO;
  glGenBuffers(1, &instanceColorVBO);
  glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * instanceCount,
               &cubeColors[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glVertexAttribDivisor(5, 1);
  glBindVertexArray(0); // Unbind the VAO.

  // --- Section 6: FBO Initialization ---
  IdPickingFBO pickingFBO;
  pickingFBO.Init(SCR_WIDTH, SCR_HEIGHT);
  pPickingFBO = &pickingFBO;

  // --- Section 7: Render Loop ---
  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {
    // --- SINGLE RENDER PASS ---
    // 1. Render to our custom FBO.
    pickingFBO.BindForWriting();
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // 2. Clear all buffers.
    glClearColor(0.1f, 0.1f, 0.1f,
                 1.0f); // Set clear color for the color buffer.
    const GLuint clearID[4] = {0, 0, 0, 0};
    glClearBufferuiv(
        GL_COLOR, 1,
        clearID); // Clear the integer ID buffer (attachment 1) to 0.
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT); // Clear the color buffer (attachment 0) and
                                  // depth buffer.

    // 3. Set uniforms and draw.
    combinedShader.use();
    glm::mat4 projection =
        glm::perspective(glm::radians(45.0f),
                         (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    combinedShader.setMat4("projection", projection);
    combinedShader.setMat4("view", view);

    glBindVertexArray(cubeVAO);
    // This single call draws all 3 cubes.
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceCount);
    glBindVertexArray(0);

    // --- Blit FBO to Screen ---
    // 4. Copy the final image from our FBO to the screen's default framebuffer.
    pickingFBO.BindForBlitting();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // The screen is the draw target.
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO.

    // 5. Swap buffers and poll events.
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // --- Section 8: Cleanup ---
  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteBuffers(1, &instanceMatrixVBO);
  glDeleteBuffers(1, &instanceColorVBO);
  glfwTerminate();
  return 0;
}

// --- Callback Functions ---
// Called whenever the window is resized.
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  SCR_WIDTH = width;
  SCR_HEIGHT = height;
  // It's crucial to re-initialize the FBO with the new dimensions.
  if (pPickingFBO) {
    pPickingFBO->Init(width, height);
  }
}

// Called on a mouse button event.
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (pPickingFBO) {
      // Read the ID from the FBO at the cursor's position.
      // Y-coordinate is flipped because GLFW's origin is top-left, while
      // OpenGL's is bottom-left.
      unsigned int clickedID =
          pPickingFBO->ReadPixel((int)xpos, SCR_HEIGHT - 1 - (int)ypos);

      if (clickedID == 0) {
        std::cout << "Clicked on the background." << std::endl;
      } else {
        std::cout << "Clicked on object with ID: " << clickedID << " (Color: "
                  << (clickedID == 1 ? "Coral"
                                     : (clickedID == 2 ? "Green" : "Blue"))
                  << ")" << std::endl;
      }
    }
  }
}
