#ifdef VERTEX_SHADER
layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec4 vColor;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

out vec4 vOutColor;

void
main()
{
    vOutColor   = vColor;
    gl_Position = ProjectionMatrix * ViewMatrix * vec4(vPosition, 0, 1);
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
