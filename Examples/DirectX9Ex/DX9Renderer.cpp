#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "DX9Renderer.h"

#pragma comment(lib, "d3d9.lib" )
#pragma comment(lib, "d3dx9.lib" )

#define SRELEASE( ptr ) if( ptr ) { ptr->Release(); ptr = NULL; }
DX9Renderer::DX9Renderer( HWND hWnd )
: m_hWnd( hWnd )
, m_d3d9( NULL )
, m_device( NULL )
, m_frameBuf( NULL )
, m_bCoInit( FALSE )
{
	if( 0 == m_hWnd )
		throw -1;

	if( SUCCEEDED( ::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY ) ) )
		m_bCoInit = TRUE;
	else
		throw -2;

	{
		D3DPRESENT_PARAMETERS p = {0};
		p.hDeviceWindow = m_hWnd;
		p.Windowed = TRUE;
		p.SwapEffect = D3DSWAPEFFECT_DISCARD;

		if( SUCCEEDED( Direct3DCreate9Ex( D3D_SDK_VERSION, &m_d3d9) ) &&
			SUCCEEDED( m_d3d9->CreateDeviceEx( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p, NULL, &m_device  ) ) )
		{
            // Create our 'render target'
            //if (FAILED(::D3DXCreateTextureFromFile( m_device, _T("earth.jpg"), &m_frameBuf )))
            if (FAILED(::D3DXCreateTextureFromFile( m_device, _T("..\\Res\\earth.jpg"), &m_frameBuf )))
            {
				throw -6;
		}
        }
		else
			throw -3;
	}
}

DX9Renderer::~DX9Renderer()
{
	SRELEASE( m_frameBuf );
	SRELEASE( m_device );
	SRELEASE( m_d3d9 );
	if( m_bCoInit )
		::CoUninitialize();
}

HRESULT DX9Renderer::draw( D3DXMATRIX const& view, D3DMATRIX const& proj )
{
    // Generate some output.
    // In this sample, the frame buffer (m_frameBuf) is constant
    return D3D_OK;
}

HRESULT DX9Renderer::present()
{
	return m_device->PresentEx( NULL, NULL, NULL, NULL, 0 );
}

