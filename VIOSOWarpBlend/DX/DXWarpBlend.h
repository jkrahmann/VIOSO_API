#pragma once

#include "../common.h"
#include <D3Dcompiler.h>
//#include <DxErr.h>
#include "../3rdparty/d3dX/Include/d3dx9math.h"
struct DXSCREENVERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex1;

	static const DWORD FVF;
};


class DXWarpBlend : public VWB_Warper_base
{
public:

	typedef __declspec( align(4) ) struct PosT {FLOAT x,y,z;} PosT;
	typedef __declspec( align(4) ) struct TexT { FLOAT u,v; } TexT;
	typedef __declspec( align(4) ) struct ColT { FLOAT r,g,b,a; } ColT;
	typedef __declspec( align(4) ) struct SimpleVertex
	{
		PosT Pos;
		TexT Tex;
	} SimpleVertex;

	typedef  __declspec( align( 4 ) ) struct ConstantBuffer
	{
		FLOAT matView[16];
		FLOAT border[4];
		FLOAT params[4];
		FLOAT offsScale[4];
		FLOAT offsScaleCur[4];
		FLOAT blackBias[4];
	} ConstantBuffer;

	const FLOAT _black[4] = { 0,0,0,1 };

protected:
	char					m_modelPath[MAX_PATH];  // the path to a successful loaded model
public:
	///< the constructor
	DXWarpBlend();

    ///< the destructor
	virtual ~DXWarpBlend();

	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

};
