#version 300 es
 precision mediump float;
uniform sampler2D tex;

in vec2 fragTexCoord;
out vec4 out_fragData;

void main()
{
    out_fragData = texture(tex, fragTexCoord);
}
