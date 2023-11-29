#version 450

layout(location = 0) out vec3 fragcolor;

layout(location = 0) in vec2 Position;



void main()
{
    gl_Position = vec4(Position, 0.0, 1.0);
    fragcolor = vec3(0.0, 0.0, 1.0);
}