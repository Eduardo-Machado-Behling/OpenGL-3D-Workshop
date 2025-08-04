#include "shader.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace Shader {

// Hack way, better way would be provide argv[0];
const std::filesystem::path Shader::ROOT = []() {
  std::filesystem::path cwd = std::filesystem::current_path();
  std::filesystem::path assets = cwd / "bin" / "assets" / "shaders";

  if (!std::filesystem::exists(assets))
    assets = cwd / "assets" / "shaders";

  return assets;
}();

const std::unordered_map<Shader::ShaderType, std::string> Shader::EXTS = {
    {ShaderType::VERTEX, ".vert"},  {ShaderType::FRAGMENT, ".frag"},
    {ShaderType::GEOMETRY, ".geo"}, {ShaderType::TCC, ".tcc"},
    {ShaderType::TCE, ".tce"},
};

const std::unordered_map<std::string, Shader::ShaderType> Shader::REXTS = {
    {".vert", ShaderType::VERTEX},  {".frag", ShaderType::FRAGMENT},
    {".geo", ShaderType::GEOMETRY}, {".tcc", ShaderType::TCC},
    {".tce", ShaderType::TCE},
};

Shader::ShaderType Shader::getTypeFromString(const char *shader) {
  return REXTS.at(std::strrchr(shader, '.'));
}

Shader::Shader(const char *shader) {
  this->type = getTypeFromString(shader);
  std::filesystem::path shaderPath = ROOT / shader;
  createShader(shaderPath);
}

Shader::Shader(const char *shader, Shader::ShaderType type) {
  this->type = type;
  std::filesystem::path shaderPath = ROOT / (shader + EXTS.at(type));
  createShader(shaderPath);
}

Shader::ShaderType Shader::getType() { return this->type; }
const std::filesystem::path Shader::getPath() { return this->path; }

void Shader::createShader(const std::filesystem::path &path) {
  std::ifstream sourceCodeFile(path, std::ios::ate);
  size_t size = sourceCodeFile.tellg();

  char *sourceCode = (char *)malloc(size * sizeof(*sourceCode) + 1);
  sourceCodeFile.seekg(0);
  sourceCodeFile.read(sourceCode, size);
  sourceCode[size] = '\0';

  std::cout << "Source: \n " << sourceCode << "\n\n";
  id = glCreateShader((GLenum)(this->type));
  glShaderSource(id, 1, &sourceCode, NULL);
  glCompileShader(id);

  GLint success;
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);

  if (success == GL_FALSE) {
    // 5. Get the length of the info log
    GLint logLength;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength > 0) {
      // Allocate a buffer and retrieve the info log
      char *info = (char *)malloc(logLength);
      glGetShaderInfoLog(id, logLength, NULL, info);

      // Print the error message
      std::cerr << "ERROR::SHADER::COMPILATION_FAILED\nFile: " << path << "\n"
                << info << std::endl;

      throw new std::runtime_error("Shader compilation Error");
    }
  }

  this->path = path;
}

Shader::~Shader() { glDeleteShader(id); }

Program::Program() { id = glCreateProgram(); }
Program::~Program() { glDeleteProgram(id); }

Program &Program::attachShader(const char *shader, Shader::ShaderType type) {
  return *this;
}

Program &Program::attachShader(const char *shader) {
  Shader::ShaderType type = Shader::getTypeFromString(shader);
  shaders.emplace(type, shader);
  return *this;
}
Program &Program::release() {
  shaders.clear();
  return *this;
}

Program &Program::link() {
  for (auto &[type, shader] : shaders) {
    glAttachShader(id, shader.id);
  }

  glLinkProgram(id);

  GLint success;
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (success == GL_FALSE) {
    GLint logLength;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
      char *info = (char *)malloc(logLength);
      glGetProgramInfoLog(id, logLength, NULL, info);
      std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << info << std::endl;
      throw new std::runtime_error("Program linking error! ");
    }
  }
  return *this;
}

Program &Program::bind() {
  glUseProgram(id);
  return *this;
}

GLuint Program::getLocation(const char *name) {
  return glGetUniformLocation(id, name);
}

} // namespace Shader
