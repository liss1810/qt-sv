#version 330 core
//#version 300 es
//#ifdef GL_FRAGMENT_PRECISION_HIGH
//  precision highp float;
//#else
//  precision mediump float;
//#endif

in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D myTexture;
uniform sampler2D myMask;
uniform vec4 myGain;
void main()
{
//    fragColor = vec4(texture(myTexture, TexCoord).bgr, texture(myMask, TexCoord).r) * myGain;
    fragColor = vec4(texture(myTexture, TexCoord).rgb, /*texture(myMask, TexCoord).r*/1.0);
}
