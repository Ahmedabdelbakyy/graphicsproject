#define GL_SILENCE_DEPRECATION
#include "Model_OBJ.h"
#include <iostream>
#include <fstream>
#include <sstream>

Model_OBJ::Model_OBJ() {
    texID = 0; // <--- FIX: Initialize variables
}

void Model_OBJ::Load(const char* filename, GLuint textureID) {
    // --- FIX: Clear old data to prevent double-loading glitches ---
    vertices.clear();
    texCoords.clear();
    normals.clear();
    faces.clear();
    // -------------------------------------------------------------

    this->texID = textureID;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Could NOT find file at: " << filename << std::endl;
        return;
    }

    std::cout << "SUCCESS: Opened " << filename << ". Parsing..." << std::endl;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            texCoords.push_back(u); texCoords.push_back(v);
        }
        else if (type == "vn") {
            float nx, ny, nz;
            iss >> nx >> ny >> nz;
            normals.push_back(nx); normals.push_back(ny); normals.push_back(nz);
        }
        else if (type == "f") {
            std::vector<std::string> segments;
            std::string segment;
            while (iss >> segment) segments.push_back(segment);

            int triangleIndices[2][3] = { {0, 1, 2}, {0, 2, 3} };
            int numTriangles = (segments.size() >= 4) ? 2 : 1;

            for (int t = 0; t < numTriangles; t++) {
                Face face;
                for (int i = 0; i < 3; i++) {
                    std::string seg = segments[triangleIndices[t][i]];
                    std::stringstream ss(seg);
                    std::string val;

                    if (std::getline(ss, val, '/')) face.vIndex[i] = std::stoi(val) - 1;
                    if (std::getline(ss, val, '/')) face.tIndex[i] = val.empty() ? 0 : std::stoi(val) - 1;
                    if (std::getline(ss, val, '/')) face.nIndex[i] = val.empty() ? 0 : std::stoi(val) - 1;
                }
                faces.push_back(face);
            }
        }
    }
    file.close();
    std::cout << "DONE: Loaded " << faces.size() << " faces." << std::endl;
}

void Model_OBJ::Draw() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        for (int i = 0; i < 3; i++) {
            // FIX: Cast to (size_t) to fix "signed/unsigned mismatch" warnings
            if (!normals.empty() && (size_t)face.nIndex[i] < normals.size() / 3)
                glNormal3f(normals[face.nIndex[i] * 3], normals[face.nIndex[i] * 3 + 1], normals[face.nIndex[i] * 3 + 2]);

            if (!texCoords.empty() && (size_t)face.tIndex[i] < texCoords.size() / 2)
                glTexCoord2f(texCoords[face.tIndex[i] * 2], texCoords[face.tIndex[i] * 2 + 1]);

            if (!vertices.empty() && (size_t)face.vIndex[i] < vertices.size() / 3)
                glVertex3f(vertices[face.vIndex[i] * 3], vertices[face.vIndex[i] * 3 + 1], vertices[face.vIndex[i] * 3 + 2]);
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}