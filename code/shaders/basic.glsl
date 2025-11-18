#define U32_MAX 0x7fffffff

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vUVData;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec3 vVSNormals;
layout(location = 4) in uint vTextureIndex;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform uint uEffectMask;

out      vec4 vOutColor;
out      vec2 vOutUVData;
out flat uint vOutTextureIndex;
out flat uint vOutEffectMask;

void
main()
{
    vOutColor        = vColor;
    vOutUVData       = vUVData;
    vOutTextureIndex = vTextureIndex;
    vOutEffectMask   = uEffectMask;
    gl_Position      = uProjectionMatrix * uViewMatrix * vec4(vPosition, 1);
}
#endif

#ifdef FRAGMENT_SHADER

in       vec4 vOutColor;
in       vec2 vOutUVData;
in  flat uint vOutTextureIndex;
in  flat uint vOutEffectMask;

layout(location = 0) out vec4 vFragColor;
layout(location = 1) out vec4 vTestColor;

uniform sampler2D uTest;

void
main()
{
    vec4 TextureColor = vec4(1.0);
    if(vOutTextureIndex != U32_MAX)
    {
        TextureColor = texelFetch(uTest, ivec2(vOutUVData), 0);
        if(TextureColor.a == 0.0)
        {
            discard;
        }
    }
    vFragColor = TextureColor * vOutColor;
    vTestColor = vec4(vOutEffectMask);
}
#endif
