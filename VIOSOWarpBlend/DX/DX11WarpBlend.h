#pragma once

#include "DXWarpBlend.h"
#include <D3D11.h>

class DX11WarpBlend : public DXWarpBlend
{
public:

protected:
    ID3D11Device*				m_device;           // the d3d device
	ID3D11DeviceContext*		m_dc;				// the d3d devicecontext
    ID3D11ShaderResourceView*	m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
    ID3D11ShaderResourceView*	m_texBlend;         // the blend lookup texture view
	ID3D11ShaderResourceView*	m_texBB;			// backbuffer copy or source texture view
	ID3D11ShaderResourceView*	m_texWarpCalc;		// the caclulated warp after model inscription
	ID3D11ShaderResourceView*	m_texCur;			// the mouse cursor texture
	D3D11_VIEWPORT				m_vp;				// the viewport, if width and height is set, use it on begin of rendering
	HWND						m_focusWnd;			// the focus window handle

	ID3D11VertexShader*			m_VertexShader;
	ID3D11PixelShader*			m_PixelShader;
	ID3D11Buffer*				m_VertexBuffer;		// the 
    ID3D11Buffer*				m_VertexBufferModel; // the vertex buffer of a model
    ID3D11Buffer*				m_IndexBufferModel; // the index buffer to that vertex buffer of a model
	ID3D11SamplerState*			m_SSLin;
	ID3D11SamplerState*			m_SSClamp;
	ID3D11Buffer*				m_ConstantBuffer;
	ID3D11RasterizerState*		m_RasterState;
	ID3D11InputLayout*			m_Layout;

	// render states
	ID3D11DepthStencilState*	m_DepthState;
	ID3D11BlendState*			m_BlendState;
public:
	///< the constructor
	DX11WarpBlend( ID3D11Device* pDevice );

    ///< the destructor
	virtual ~DX11WarpBlend();

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
};
