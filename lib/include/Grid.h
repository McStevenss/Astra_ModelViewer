#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.hpp"

class Grid {
public:
    Grid() = default;
    Grid(int size, float spacing = 1.0f);

    void Render(Shader& shader);

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLsizei indexCount = 0;

    glm::mat4 model = glm::mat4(1.0f);
};