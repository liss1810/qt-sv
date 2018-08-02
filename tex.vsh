#version 300 es

//vertex attributes
layout(location = 0) in vec2 in_ScreenCoord;
layout(location = 1) in vec2 in_TexCoord;
out vec2 fragTexCoord;

void main()
{
        gl_Position = vec4(in_ScreenCoord, 0.0, 1.0);
        fragTexCoord = in_TexCoord;
}
