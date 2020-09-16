//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_samplerLin : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
	float4x4 matWorld;
	float4x4 matView;
	float4x4 matProj;
};

PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD)
{
    PSInput result;

//    result.position = position;
    result.position = mul( position, matWorld );
    result.position = mul( result.position, matView );
    result.position = mul( result.position, matProj );
   // result.position.xyz /= result.position.w;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample( g_samplerLin, input.uv);
}
