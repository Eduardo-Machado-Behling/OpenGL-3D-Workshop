#ifndef MODEL_HPP
#define MODEL_HPP

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "mesh.hpp"
#include "shader.hpp"

#include <iostream>
#include <string>
#include <vector>

class Model {
public:
  // Model Data
  std::vector<Mesh> meshes;

  /**
   * @brief The public static factory method to load a model from a file.
   * @param path The file path to the 3D model.
   * @return A fully loaded Model object.
   */
  static Model Load(const std::string &path);

  /**
   * @brief Draws all the meshes in the model.
   * @param shader The shader program to use for drawing.
   */
  void Draw(Shader::Program &shader);

private:
  /**
   * @brief Private constructor. Models should only be created via the static
   * Load method.
   * @param meshes A vector of meshes to be moved into the new Model object.
   */
  Model(std::vector<Mesh> &&meshes);
};

#endif
