#version 300 es
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vTexCoord;
out vec2 TexCoord;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp * vec4(vPosition.xyz, 1);
    TexCoord = vTexCoord;
}
