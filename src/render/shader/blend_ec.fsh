#version 300 es
precision mediump float;
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D myTexture;
uniform sampler2D myMask;
uniform vec4 myGain;
void main()
{
//    fragColor = vec4(texture(myTexture, TexCoord).bgr, texture(myMask, TexCoord).r) * myGain; \n "
    fragColor = vec4(texture(myTexture, TexCoord).bgr, texture(myMask, TexCoord).r);
}
