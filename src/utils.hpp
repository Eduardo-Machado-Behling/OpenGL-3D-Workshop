#pragma once

#include <vector>
#include <string>

std::string ReadFile(const std::string& filePath);

unsigned int LoadTexture(const std::string& texture_path);

// bool LoadObj(const std::string& obj_file_path, std::vector<Mesh::Vertex>& vertices, std::vector<unsigned int>& indices);