#version 410 core
in vec3 Normal;
out vec4 FragColor;

void main()
{
    // visualize normals as colors
    FragColor = vec4(Normal * 0.5 + 0.5, 1.0);
}