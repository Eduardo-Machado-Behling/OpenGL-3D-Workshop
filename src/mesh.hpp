#ifndef MESH_HPP
#define MESH_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp" // Assuming your shader class is in this header

#include <string>
#include <vector>

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
};

struct Texture {
  unsigned int id;
  std::string type; // e.g., "texture_diffuse", "texture_specular"
  std::string
      path; // We store the path of the texture to compare with other textures
};

class Mesh {
public:
  // Mesh data
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  unsigned int VAO;

  // Constructor
  Mesh(const std::vector<Vertex> &vertices,
       const std::vector<unsigned int> &indices,
       const std::vector<Texture> &textures);

  // Renders the mesh
  void Draw(Shader::Program &shader);

private:
  // Render data
  unsigned int VBO, EBO;

  // Initializes all the buffer objects/arrays
  void setupMesh();
};

#endif
