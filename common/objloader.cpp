#include "objloader.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide :
// - Binary files. Reading a model should be just a few memcpy's away, not
// parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(const char *path, std::vector<glm::vec3> &out_vertices,
             std::vector<glm::vec2> &out_uvs,
             std::vector<glm::vec3> &out_normals) {
    printf("Loading OBJ file %s...\n", path);

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::ifstream infile;
    std::string line;
    infile.open(path);

    while (std::getline(infile, line)) {
        std::istringstream ss(line);
        std::string head;

        ss >> head;

        if (head == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        } else if (head == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            uv.y = -uv.y; // Invert V coordinate since we will only use DDS
                          // texture, which are inverted. Remove if you want to
                          // use TGA or BMP loaders.
            temp_uvs.push_back(uv);
        } else if (head == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        } else if (head == "f") {
            std::vector<unsigned int> vertexIndex, uvIndex, normalIndex;
            std::string s;
            while (ss >> s) {
                int x;
                std::vector<unsigned int> vun;
                std::replace(s.begin(), s.end(), '/', ' ');
                std::stringstream ss2(s);
                while (ss2 >> x) {
                    vun.push_back(x);
                }
                if (vun.size() == 3) {
                    vertexIndex.push_back(vun[0]);
                    uvIndex.push_back(vun[1]);
                    normalIndex.push_back(vun[2]);
                } else {
                    std::cout << "Cannot parse following line: \n"
                              << line << std::endl;
                }
            }
            if (!(vertexIndex.size() == uvIndex.size() &&
                  vertexIndex.size() == normalIndex.size()))
                return false;

            for (int i = 2; i < vertexIndex.size(); i++) {
                vertexIndices.push_back(vertexIndex[0]);
                vertexIndices.push_back(vertexIndex[i - 1]);
                vertexIndices.push_back(vertexIndex[i]);
                uvIndices.push_back(uvIndex[0]);
                uvIndices.push_back(uvIndex[i - 1]);
                uvIndices.push_back(uvIndex[i]);
                normalIndices.push_back(normalIndex[0]);
                normalIndices.push_back(normalIndex[i - 1]);
                normalIndices.push_back(normalIndex[i]);
            }
        } else if (head == "#") {
            ;
        } else if (head == "s") {
            ;
        } else {
            std::cout << "Cannot parse following line: \n" << line << std::endl;
        }
    }

    // For each vertex of each triangle
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {

        // Get the indices of its attributes
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];

        // Get the attributes thanks to the index
        glm::vec3 vertex = temp_vertices[vertexIndex - 1];
        glm::vec2 uv = temp_uvs[uvIndex - 1];
        glm::vec3 normal = temp_normals[normalIndex - 1];

        // Put the attributes in buffers
        out_vertices.push_back(vertex);
        out_uvs.push_back(uv);
        out_normals.push_back(normal);
    }
    std::cout << "vertexIndices.size() = " << vertexIndices.size() << std::endl;
    return true;
}
