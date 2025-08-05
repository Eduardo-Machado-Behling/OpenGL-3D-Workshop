#include "model.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- Anonymous Namespace for Static Helper Functions ---
// These functions perform the actual loading logic. They are hidden from other
// files.
namespace {

unsigned int TextureFromFile(const char *path, const std::string &directory);
std::vector<Texture>
loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                     const std::string &typeName, const std::string &directory,
                     std::vector<Texture> &textures_loaded);
Mesh processMesh(aiMesh *mesh, const aiScene *scene,
                 const std::string &directory,
                 std::vector<Texture> &textures_loaded);
void processNode(aiNode *node, const aiScene *scene,
                 std::vector<Mesh> &outMeshes, const std::string &directory,
                 std::vector<Texture> &textures_loaded);

// --- Function Implementations ---

void processNode(aiNode *node, const aiScene *scene,
                 std::vector<Mesh> &outMeshes, const std::string &directory,
                 std::vector<Texture> &textures_loaded) {
  // Process all the node's meshes
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *assimpMesh = scene->mMeshes[node->mMeshes[i]];
    outMeshes.push_back(
        processMesh(assimpMesh, scene, directory, textures_loaded));
  }
  // Recurse on each of the children
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene, outMeshes, directory,
                textures_loaded);
  }
}

Mesh processMesh(aiMesh *mesh, const aiScene *scene,
                 const std::string &directory,
                 std::vector<Texture> &textures_loaded) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    vertex.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                       mesh->mVertices[i].z};
    if (mesh->HasNormals()) {
      vertex.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                       mesh->mNormals[i].z};
    }
    if (mesh->mTextureCoords[0]) {
      vertex.TexCoords = {mesh->mTextureCoords[0][i].x,
                          mesh->mTextureCoords[0][i].y};
    } else {
      vertex.TexCoords = glm::vec2(0.0f, 0.0f);
    }
    vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  std::vector<Texture> diffuseMaps =
      loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse",
                           directory, textures_loaded);
  textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
  std::vector<Texture> specularMaps =
      loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular",
                           directory, textures_loaded);
  textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

  return Mesh(vertices, indices, textures);
}

std::vector<Texture>
loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                     const std::string &typeName, const std::string &directory,
                     std::vector<Texture> &textures_loaded) {
  std::vector<Texture> textures;
  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString str;
    mat->GetTexture(type, i, &str);
    bool skip = false;
    for (unsigned int j = 0; j < textures_loaded.size(); j++) {
      if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
        textures.push_back(textures_loaded[j]);
        skip = true;
        break;
      }
    }
    if (!skip) {
      Texture texture;
      texture.id = TextureFromFile(str.C_Str(), directory);
      texture.type = typeName;
      texture.path = str.C_Str();
      textures.push_back(texture);
      textures_loaded.push_back(texture);
    }
  }
  return textures;
}

unsigned int TextureFromFile(const char *path, const std::string &directory) {
  std::string filename = std::string(path);
  filename = directory + '/' + filename;

  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    std::cout << "Texture failed to load at path: " << path << std::endl;
  }
  stbi_image_free(data);
  return textureID;
}
} // End of anonymous namespace

// --- Public Static Method ---
Model Model::Load(const std::string &path) {
  std::vector<Mesh> loadedMeshes;
  std::vector<Texture> loadedTextures; // For caching

  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
                                  aiProcess_CalcTangentSpace);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    throw std::runtime_error("Model fail");
  }

  std::string directory = path.substr(0, path.find_last_of('/'));

  // Begin the recursive processing
  processNode(scene->mRootNode, scene, loadedMeshes, directory, loadedTextures);

  // Use the private constructor to create the model object, moving the loaded
  // data into it.
  return Model(std::move(loadedMeshes));
}

// --- Private Constructor ---
Model::Model(std::vector<Mesh> &&meshes) : meshes(std::move(meshes)) {}

// --- Public Draw Method ---
void Model::Draw(Shader::Program &shader) {
  for (unsigned int i = 0; i < meshes.size(); i++) {
    meshes[i].Draw(shader);
  }
}
