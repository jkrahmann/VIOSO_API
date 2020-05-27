#pragma once

#if defined( _DEBUG )
#define D3D_DEBUG_INFO
#endif

#include <d3d9.h>
#include "../../VIOSOWarpBlend/3rdparty/d3dX/Include/d3dx9.h"

class DX9Renderer
{
protected:
    LPDIRECT3D9EX       m_d3d9;             // the d3d object
    LPDIRECT3DDEVICE9EX   m_device;           // the d3d device
    LPDIRECT3DTEXTURE9  m_frameBuf;         // the rendering output to be filled by the host process

    HWND m_hWnd;                            // handle of the output window
    BOOL m_bCoInit;                         // indicates successful CoInitialize()

public:
    DX9Renderer( HWND hWnd );
    ~DX9Renderer();
    HRESULT draw( D3DXMATRIX const& view, D3DMATRIX const& proj );
    HRESULT present();
    LPDIRECT3DDEVICE9EX getDevice() { return m_device; }
    LPDIRECT3DTEXTURE9 getOutput() { return m_frameBuf; }
};


