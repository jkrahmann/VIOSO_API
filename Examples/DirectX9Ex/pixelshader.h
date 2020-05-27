static char s_pixelShader[] =
"                                               \
Texture2D texContent : register(t0);            \
Texture2D texWarp : register(t1);               \
Texture2D texBlend : register(t2);              \
                                                \
float4x4 matView : register(c0);                \
float4 bBorder : register(c4);                  \
                                                \
sampler samContent = sampler_state              \
{                                               \
    Texture = <texContent>;                     \
    MipFilter = LINEAR;                         \
    MinFilter = LINEAR;                         \
    MagFilter = LINEAR;                         \
    AddressV = BORDER;                          \
    AddressU = BORDER;                          \
    BorderColor = float4(0,0,0,0);              \
};                                              \
                                                \
sampler samWarp = sampler_state                 \
{                                               \
    Texture = <texWarp>;                        \
    MipFilter = LINEAR;                         \
    MinFilter = LINEAR;                         \
    MagFilter = LINEAR;                         \
    AddressV = MIRROR;                          \
    AddressU = MIRROR;                          \
    BorderColor = float4(0,0,0,0);              \
};                                              \
                                                \
sampler samBlend = sampler_state                \
{                                               \
    Texture = <texBlend>;                       \
    MipFilter = LINEAR;                         \
    MinFilter = LINEAR;                         \
    MagFilter = LINEAR;                         \
    AddressV = BORDER;                          \
    AddressU = BORDER;                          \
    BorderColor = float4(0,0,0,0);              \
};                                              \
                                                \
struct VS_OUT {                                 \
    float4 pos : POSITION;                      \
    float2 tex : TEXCOORD0; };                  \
                                                \
float4 TST( VS_OUT vIn ) : COLOR                \
{                                               \
    float4 color = float4(1,0,1,1);             \
    color.xy = vIn.tex.xy;                      \
    return color;                               \
}                                               \
                                                \
float4 PS( VS_OUT vIn ) : COLOR                 \
{                                               \
    float4 color = tex2D(samContent, vIn.tex ); \
    return color;                               \
}                                               \
                                                \
float4 PSWB( VS_OUT vIn ) : COLOR               \
{                                               \
    float4 tex = tex2D( samWarp, vIn.tex );     \
    float4 blend = tex2D( samBlend, vIn.tex );  \
    if( bBorder.x > 0.5f )                      \
    {                                           \
        tex.x*= 1.02;                           \
        tex.x-= 0.01;                           \
        tex.y*= 1.02;                           \
        tex.y-= 0.01;                           \
    }                                           \
    float4 vOut = tex2D( samContent, tex.xy );  \
    vOut.rgb*= 1 == tex.z || 1 == tex.w ? blend.rgb : 0;        \
    vOut.a = 1;                                 \
    return vOut;                                \
}                                               \
                                                \
float4 PSWB3D( VS_OUT vIn ) : COLOR             \
{                                               \
    float4 tex = tex2D( samWarp, vIn.tex );     \
    float4 blend = tex2D( samBlend, vIn.tex );  \
    float4 vOut = float4( 0,0,0,1);             \
    if( 1 == tex.w )                            \
    {                                           \
        tex = mul( tex, matView );              \
        tex.xy/= tex.w;                         \
        tex.x/=2;                              \
        tex.y/=-2;                               \
        tex.xy+= 0.5;                           \
		if( 0 < tex.z )							\
		{										\
			vOut = tex2D( samContent, tex.xy ); \
			vOut.rgb*= blend.rgb;               \
		}										\
    }                                           \
    return vOut;                                \
}                                               \
";
