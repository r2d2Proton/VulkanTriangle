#version 450

// inputs
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec4 inColor;

// outputs
layout (location = 0) out vec4 fragColor;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
