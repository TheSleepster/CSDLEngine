struct point_light
{
    vec2  ws_position;
    vec2  direction;
    vec4  light_color;
    float radius;
    float cone_angle;
    float strength;
};

#define PI 3.1415926535

#ifdef VERTEX_SHADER
layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vUVData;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec3 vVSNormals;
layout(location = 4) in uint vTextureIndex;

uniform mat4 uProjectionMatrix;

out      vec4 vOutColorMask;
out      vec2 vOutLocalSpace;
out      vec2 vOutAtlasUVs;
out flat uint InstanceID;

void
main()
{
    vOutColorMask  = vColor;
    vOutLocalSpace = vVSNormals.xy;
    vOutAtlasUVs   = vUVData;
    InstanceID     = vTextureIndex;

    gl_Position = uProjectionMatrix * vec4(vPosition, 0, 1);
}
#endif

#ifdef FRAGMENT_SHADER
layout(std430, binding = 0) buffer PointLightSBO
{
    point_light PointLights[];
};

in      vec4 vOutColorMask;
in      vec2 vOutLocalSpace;
in      vec2 vOutLocalUVs;
in flat uint InstanceID;

out vec4 vFragColor;

void
main()
{
    const float TileSize = 4.0;
    const uint  TextureSize = 4096;
    const uint  CellSize    = 256;

    point_light Light = PointLights[InstanceID];

    vec2 FragPos         = gl_FragCoord.xy;
    vec2 LightCellCenter = floor(FragPos / CellSize) * CellSize + vec2(CellSize * 0.5);
    vec2 FragTilePos     = floor(FragPos / TileSize) * TileSize + TileSize * 0.5;

    vec2 LightPos     = FragTilePos - LightCellCenter;
    vec2 LightDir     = normalize(vec2(LightPos - FragTilePos));
    vec2 SpotlightDir = normalize(Light.direction);

    float LightDist    = length(LightPos);
    if(LightDist > Light.radius) return;

    vec2  FragToLight  = FragPos - LightPos;
    float cosTheta     = dot(SpotlightDir, normalize(FragToLight));
    float cosSpotAngle = cos(Light.cone_angle);

    float SpotEffect;
    float EdgeDelta = 0.087;
    if(Light.cone_angle < PI)
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
    vec4  LightContrib        = (1.0 - exp(-0.5 * SpotStrength)) * vOutColorMask;
    vec4  EffectiveLightColor = LightContrib;

    vFragColor = EffectiveLightColor;
}
#endif
