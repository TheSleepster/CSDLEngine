// Common header
struct PointLight {
    float2 ws_position;
    float2 direction;
    float4 light_color;
    float  radius;
    float  cone_angle;
    float  strength;
};

#define PI 3.1415926535

// ------------------
// Vertex shader
// ------------------

// Input layout (vertex attributes)
struct VSInput {
    float2 vPosition      : ATTRIB0;
    float2 vUVData        : ATTRIB1;
    float4 vColor         : ATTRIB2;
    float3 vVSNormals     : ATTRIB3;
    uint   vTextureIndex  : ATTRIB4;
};

// Outputs to the fragment shader
struct VSOutput {
    float4 vOutColorMask  : TEXCOORD0;
    float2 vOutLocalSpace : TEXCOORD1;
    float2 vOutAtlasUVs   : TEXCOORD2;
    uint   InstanceID     : TEXCOORD3;   // no `flat` in HLSL/Slang, semantics suffice
    float4 position       : SV_Position; // replaces gl_Position
};

// Uniforms
cbuffer Globals : register(b0)
{
    float4x4 uProjectionMatrix;
}

VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.vOutColorMask  = input.vColor;
    output.vOutLocalSpace = input.vVSNormals.xy;
    output.vOutAtlasUVs   = input.vUVData;
    output.InstanceID     = input.vTextureIndex;

    output.position = mul(uProjectionMatrix, float4(input.vPosition, 0, 1));
    return output;
}

// ------------------
// Fragment shader
// ------------------

StructuredBuffer<PointLight> PointLights : register(t0);

float4 fragmentMain(VSOutput input) : SV_Target0
{
    const float TileSize   = 4.0;
    const uint  TextureSize = 4096;
    const uint  CellSize    = 256;

    PointLight Light = PointLights[input.InstanceID];

    float2 FragPos         = float2(input.position.xy); // SV_Position.xy in HLSL
    float2 LightCellCenter = floor(FragPos / CellSize) * CellSize + float2(CellSize * 0.5, CellSize * 0.5);
    float2 FragTilePos     = floor(FragPos / TileSize) * TileSize + TileSize * 0.5;

    float2 LightPos     = FragTilePos - LightCellCenter;
    float2 LightDir     = normalize(LightPos - FragTilePos);
    float2 SpotlightDir = normalize(Light.direction);

    float LightDist = length(LightPos);
    if (LightDist > Light.radius)
        discard;

    float2 FragToLight  = FragPos - LightPos;
    float cosTheta      = dot(SpotlightDir, normalize(FragToLight));
    float cosSpotAngle  = cos(Light.cone_angle);

    float SpotEffect;
    float EdgeDelta = 0.087;
    if (Light.cone_angle < PI)
    {
        float OuterAngle    = Light.cone_angle;
        float InnerAngle    = max(0.0, Light.cone_angle - EdgeDelta);

        float cosOuter      = cos(OuterAngle);
        float cosInner      = cos(InnerAngle);

        float AngularEffect = smoothstep(cosOuter, cosInner, cosTheta);
        float RadialEffect  = smoothstep(Light.radius, 0.0, LightDist);

        SpotEffect = AngularEffect * RadialEffect;
    }
    else
    {
        SpotEffect = pow(1 - LightDist / Light.radius, 2.5);
    }

    float SpotStrength        = SpotEffect * Light.strength;
    float4 LightContrib        = (1.0 - exp(-0.5 * SpotStrength)) * input.vOutColorMask;
    float4 EffectiveLightColor = LightContrib;

    return EffectiveLightColor;
}

