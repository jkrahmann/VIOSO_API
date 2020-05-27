#pragma once

#include "DXWarpBlend.h"
#include <D3D10.h>

class DX10WarpBlend : public DXWarpBlend
{
public:

protected:
    ID3D10Device*				m_device;           // the d3d device
    ID3D10ShaderResourceView*	m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
	ID3D10ShaderResourceView* m_texBlend;         // the blend lookup texture view
	ID3D10ShaderResourceView* m_texBlack;         // the black level lookup texture view
	ID3D10ShaderResourceView*	m_texBB;			// backbuffer copy or source texture view
	ID3D10ShaderResourceView*	m_texWarpCalc;		// the caclulated warp after model inscription
	D3D10_VIEWPORT				m_vp;				// the viewport, if width and height is set, use it on begin of rendering

	ID3D10VertexShader*			m_VertexShader;
	ID3D10PixelShader*			m_PixelShader;
	ID3D10Buffer*				m_VertexBuffer;		// the 
 //   ID3D10Buffer*				m_VertexBufferModel; // the vertex buffer of a model
 //   ID3D10Buffer*				m_IndexBufferModel; // the index buffer to that vertex buffer of a model
	ID3D10SamplerState*			m_SSLin;
	ID3D10SamplerState*			m_SSClamp;
	ID3D10Buffer*				m_ConstantBuffer;
	ID3D10RasterizerState*		m_RasterState;
	ID3D10InputLayout*			m_Layout;

public:
	///< the constructor
	DX10WarpBlend( ID3D10Device* pDevice );

    ///< the destructor
	virtual ~DX10WarpBlend();

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

    virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  

protected:
	// Helper functions
    void                SetTexture(unsigned int texId, ID3D10ShaderResourceView* tex) const;
};
