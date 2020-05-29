#pragma once

#include "DXWarpBlend.h"
#include <d3d9.h>

class DX9EXWarpBlend : public DXWarpBlend
{
public:



protected:
    LPDIRECT3DDEVICE9EX       m_device;           // the d3d device
    LPDIRECT3DTEXTURE9      m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
    LPDIRECT3DTEXTURE9      m_texBlend;         // the blend lookup texture
    LPDIRECT3DTEXTURE9      m_texBlack;         // the blacklevel lookup texture
    LPDIRECT3DTEXTURE9		m_texBB;			// backbuffer copy texture
	LPDIRECT3DSURFACE9		m_srfBB;			// backbuffer copy surface
	LPDIRECT3DTEXTURE9		m_texWarpCalc;		// the caclulated warp after model inscription
	LPDIRECT3DTEXTURE9		m_texCur;			// the mouse cursor texture
	LPDIRECT3DTEXTURE9		m_texCurSysMem;			// the mouse cursor texture
	D3DVIEWPORT9			m_vp;				// the viewport, if width and height is set, use it on begin of rendering

    LPDIRECT3DPIXELSHADER9  m_PixelShader;
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;		// the 
    //LPDIRECT3DVERTEXBUFFER9 m_VertexBufferModel; // the vertex buffer of a model
    //LPDIRECT3DINDEXBUFFER9 m_IndexBufferModel; // the index buffer to that vertex buffer of a model

public:
	///< the constructor
	DX9EXWarpBlend( LPDIRECT3DDEVICE9EX pDevice );

    ///< the destructor
	virtual ~DX9EXWarpBlend();

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

 
    /** setModelDX9
	* set a 3D Model to calculate a surface 
    * @param [IN]		pModelView	it gets the updated view matrix to translate and rotate into the viewer's perspective
    * @param [IN]		path path to the model file
	* @param [IN,OPT]	nBlendChannels number of other channels to blend with
	* @param [IN,OPT]	pBlendChannels a list of handles of other channels
    * @return TRUE on success, FALSE otherwise */
	//VWB_ERROR DX9WarpBlend::setModel( D3DXMATRIX* pModelView, LPCSTR path, int nBlendChannels, DX9WarpBlend* pBlendChannels );

    virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  

protected:
	// Helper functions
    LPDIRECT3DVERTEXBUFFER9 CreateVertexBuffer(float width, float height) const;
    void                    SetTexture(unsigned int texId, LPDIRECT3DTEXTURE9 tex) const;
};
