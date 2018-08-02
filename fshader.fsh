#version 300 es
precision mediump float;
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D myTexture;
void main()
{
    fragColor = vec4(texture(myTexture, TexCoord).bgr, 1.0);
}
