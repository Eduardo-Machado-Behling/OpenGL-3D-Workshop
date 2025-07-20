#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Instanced data
layout (location = 2) in mat4 instanceMatrix;

// For Indirect drawing with SSBO
layout(std430, binding = 0) buffer ModelMatrices {
    mat4 models[];
};

uniform mat4 view;
uniform mat4 projection;
uniform bool useSSBO = false; // A uniform to switch between VBO and SSBO

void main()
{
    mat4 model;
    if (useSSBO) {
        model = models[gl_InstanceID];
    } else {
        model = instanceMatrix;
    }
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
