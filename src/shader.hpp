#ifndef SHADER_HPP
#define SHADER_HPP

#include <filesystem>
#include <glad/glad.h>
#include <string>
#include <unordered_map>

namespace Shader {

class Shader {
public:
  enum class ShaderType {
    VERTEX = GL_VERTEX_SHADER,
    TCC = GL_TESS_CONTROL_SHADER,
    TCE = GL_TESS_EVALUATION_SHADER,
    GEOMETRY = GL_GEOMETRY_SHADER,
    FRAGMENT = GL_FRAGMENT_SHADER,
  };

  Shader(const char *shader, ShaderType type);
  Shader(const char *shader);
  ~Shader();

  ShaderType getType();
  const std::filesystem::path getPath();

  static ShaderType getTypeFromString(const char *shader);

private:
  friend class Program;

  void createShader(const std::filesystem::path &path);

  static const std::filesystem::path ROOT;
  static const std::unordered_map<ShaderType, std::string> EXTS;
  static const std::unordered_map<std::string, ShaderType> REXTS;
  GLuint id;
  ShaderType type;
  std::filesystem::path path;
};

const std::unordered_map<Shader::ShaderType, GLenum> GL_SHADER_TYPES = {
    {Shader::ShaderType::VERTEX, GL_VERTEX_SHADER},
    {Shader::ShaderType::FRAGMENT, GL_FRAGMENT_SHADER},
    {Shader::ShaderType::GEOMETRY, GL_GEOMETRY_SHADER},
    {Shader::ShaderType::TCC, GL_TESS_CONTROL_SHADER},
    {Shader::ShaderType::TCE, GL_TESS_EVALUATION_SHADER},
};

class Program {
  GLuint id;
  std::unordered_map<Shader::ShaderType, Shader> shaders;

public:
  Program();
  ~Program();

  Program &attachShader(const char *shader, Shader::ShaderType type);
  Program &attachShader(const char *shader);
  Program &release();
  Program &link();
  Program &bind();

  GLuint getLocation(const char *name);
};

} // namespace Shader

#endif
