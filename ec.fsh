#version 300 es

precision mediump float;
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D myTexture;
uniform vec4 myGain;
void main()
{
//    fragColor = vec4(texture(myTexture, TexCoord).bgr, 0) * myGain;
    fragColor = vec4(texture(myTexture, TexCoord).bgr, 1.0);
}
