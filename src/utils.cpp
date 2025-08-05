#include "utils.hpp"

#include <glad/glad.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::string ReadFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string content;
    std::stringstream fileStream;
    fileStream << file.rdbuf();
    file.close();
    content = fileStream.str();
    return content;
}

GLuint LoadTexture(const std::string& texture_path) {
    GLuint textureID;
    int width, height, nrChannels;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // load image
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else // nrChannels == 4
            format = GL_RGBA;

        // load texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data);

    return textureID;
}

/*
bool LoadObj(const std::string& obj_file_path, std::vector<Mesh::Vertex>& vertices, std::vector<unsigned int>& indices) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals; 
    
    std::ifstream file(obj_file_path);
    
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") { // position
            glm::vec3 position;
            std::istringstream iss(line.substr(2));
            if (iss >> position.x >> position.y >> position.z) {
                positions.push_back(position);
            }
        }
        else if (line.substr(0, 3) == "vt ") { // UV
            glm::vec2 uv;
            std::istringstream iss(line.substr(3));
            if (iss >> uv.x >> uv.y) {
                uvs.push_back(uv);
            }
        }
        else if (line.substr(0, 3) == "vn ") { // normal
            glm::vec3 normal;
            std::istringstream iss(line.substr(3));
            if (iss >> normal.x >> normal.y >> normal.z) {
                normals.push_back(normal);
            }
        }
        else if (line.substr(0, 2) == "f ") { // face
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            std::istringstream iss(line.substr(2));

            char separator;
            for (int i = 0; i < 3; ++i) {
                if (iss >> vertexIndex[i] >> separator >> uvIndex[i] >> separator >> normalIndex[i]) {
                    vertices.push_back({positions[vertexIndex[i] - 1], uvs[uvIndex[i] - 1], normals[normalIndex[i] - 1]});
                    indices.push_back(vertices.size() - 1);
                }
            }
        }
    }
    
    file.close();

    return true;
}
*/