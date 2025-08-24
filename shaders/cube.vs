#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Normal; // pass to fragment shader
out vec3 FragPos; // optional: world position (useful for lighting)

void main()
{
    // transform vertex position
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // compute world-space fragment position
    FragPos = vec3(model * vec4(aPos, 1.0));

    // correct normal transformation
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);
}
