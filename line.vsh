#version 300 es
layout(location = 0) in vec4 vPosition;
void main()
{
    gl_Position = vPosition;
    gl_PointSize = 3.0f;
}
