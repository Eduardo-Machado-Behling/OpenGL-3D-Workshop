#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <functional>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <cstdint>

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam);

class GLFWWindow {
protected:
  GLFWWindow(uint32_t screenWidth, uint32_t screenHeight);
  ~GLFWWindow();

  GLFWwindow *window;
};

class Window : private GLFWWindow {
public:
  Window(uint32_t screenWidth, uint32_t screenHeight);

  bool shouldClose();
  void swapBuffers();
  void setTitle(const char *title);

  template <typename Func, typename... Args> void set(Func func, Args... args) {
    func(window, args...);
  }
};

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam);
#endif
