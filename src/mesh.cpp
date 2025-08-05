#include "mesh.hpp"
#include <iostream>

// Constructor implementation
Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<unsigned int> &indices,
           const std::vector<Texture> &textures) {
  this->vertices = vertices;
  this->indices = indices;
  this->textures = textures;

  // Now that we have all the required data, set the vertex buffers and its
  // attribute pointers.
  setupMesh();
}

// Initializes all the buffer objects/arrays
void Mesh::setupMesh() {
  // Create buffers/arrays
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  // Load data into vertex buffers
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0],
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               &indices[0], GL_STATIC_DRAW);

  // Set the vertex attribute pointers
  // Vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Position));
  // Vertex Normals
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Normal));
  // Vertex Texture Coords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, TexCoords));

  glBindVertexArray(0);
}

// Renders the mesh
void Mesh::Draw(Shader::Program &shader) {
  // Bind appropriate textures
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;

  for (unsigned int i = 0; i < textures.size(); i++) {
    glActiveTexture(GL_TEXTURE0 +
                    i); // Active proper texture unit before binding

    // Retrieve texture number (the N in diffuse_textureN)
    std::string number;
    std::string name = textures[i].type;
    if (name == "texture_diffuse")
      number = std::to_string(diffuseNr++);
    else if (name == "texture_specular")
      number = std::to_string(specularNr++);
    else if (name == "texture_normal")
      number = std::to_string(normalNr++);
    else if (name == "texture_height")
      number = std::to_string(heightNr++);

    // Now set the sampler to the correct texture unit
    // Note: we're using your shader class's methods
    shader.bind(); // Ensure the shader is bound before setting uniforms
    glUniform1i(shader.getLocation((name + number).c_str()), i);

    // And finally bind the texture
    glBindTexture(GL_TEXTURE_2D, textures[i].id);
  }

  // Draw mesh
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  // Always good practice to set everything back to defaults once configured.
  glActiveTexture(GL_TEXTURE0);
}
