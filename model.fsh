#version 300 es
precision mediump float;

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;

in vec3 eyePosition, eyeNormal, eyeLight;
in vec4 normPosition;

out vec4 fragColor;

vec3 lightColor = vec3(1.0);
void main()
{
    vec3 normPos = normPosition.xyz/normPosition.w;
    vec3 N = normalize(eyeNormal);
    vec3 L = normalize(eyeLight-eyePosition);
    vec3 finalColor = vec3(0.0);

    //Blin-Phong model
    finalColor = ambient;
    float lambertTerm = dot(L, N);
    if(lambertTerm >= 0.0)
    {
        finalColor += lightColor * diffuse * lambertTerm;

        //Phong model
        vec3 V = normalize(-eyePosition);
        vec3 R = reflect(-L, N);
        float spec = pow(dot(R, V), 128.0);

        if(spec >= 0.0)
        {
            finalColor += lightColor * specular * spec;
        }
    }

    fragColor = vec4(finalColor, 1.0);
}
