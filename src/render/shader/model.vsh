#version 300 es

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mvp, mv;
uniform mat3 mn;

//light
const vec3 lightPosition = vec3( 0.0, 0.0, 20.0 );

//material

out vec3 eyePosition, eyeNormal, eyeLight;
out vec4 normPosition;

void main()
{
    normPosition = mvp*vec4(position,1);
    gl_Position = normPosition;

    eyePosition = (mv*vec4(position,1)).xyz;
    eyeLight = (mv*vec4(lightPosition,1)).xyz;
    eyeNormal = normalize(mn*normal);
}
