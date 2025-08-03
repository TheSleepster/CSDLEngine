#ifdef VERTEX_SHADER
layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vUVData;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec3 vVSNormals;
layout(location = 4) in uint vTextureIndex;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

out vec4 vOutColor;

void
main()
{
    vOutColor   = vColor;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(vPosition, 0, 1);
}
#endif

#ifdef FRAGMENT_SHADER

in  vec4 vOutColor;
out vec4 vFragColor;

void
main()
{
    vFragColor = vOutColor;
}
#endif
