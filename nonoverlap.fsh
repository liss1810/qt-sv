#version 300 es
#ifdef GL_FRAGMENT_PRECISION_HIGH
  precision highp float;
#else
  precision mediump float;
#endif

//#version 330 core

in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D myTexture;
uniform vec4 myGain;
void main()
{
//        " fragColor = vec4(texture(myTexture, TexCoord).bgr, 0) * myGain; \n "
    fragColor = vec4(texture(myTexture, TexCoord).rgb, 1.0);
}
