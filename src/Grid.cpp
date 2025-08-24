#include "Grid.h"
#include <iostream>

Grid::Grid(int size, float spacing)
{
    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;

    // Generate vertices (XZ plane, Y=0)
    for (int j = 0; j <= size; ++j) {
        for (int i = 0; i <= size; ++i) {
            float x = (i - size / 2.0f) * spacing; // center the grid
            float y = 0.0f;
            float z = (j - size / 2.0f) * spacing;
            vertices.push_back(glm::vec3(x, y, z));
        }
    }

    // Generate line indices
    for (int j = 0; j <= size; ++j) {
        for (int i = 0; i < size; ++i) {
            int row = j * (size + 1);
            indices.push_back(row + i);
            indices.push_back(row + i + 1);
        }
    }
    for (int i = 0; i <= size; ++i) {
        for (int j = 0; j < size; ++j) {
            int row1 = j * (size + 1);
            int row2 = (j + 1) * (size + 1);
            indices.push_back(row1 + i);
            indices.push_back(row2 + i);
        }
    }

    indexCount = (GLsizei)indices.size();
    std::cout << "Grid index count: " << indexCount << std::endl;

    // Setup OpenGL buffers
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void Grid::Render(Shader& shader)
{
    shader.setMat4("uM", model);
    glBindVertexArray(vao);
    glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}