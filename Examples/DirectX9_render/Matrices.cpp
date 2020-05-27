//-----------------------------------------------------------------------------
// File: Matrices.cpp
//
// Desc: Now that we know how to create a device and render some 2D vertices,
//       this tutorial goes the next step and renders 3D geometry. To deal with
//       3D geometry we need to introduce the use of 4x4 matrices to transform
//       the geometry with translations, rotations, scaling, and setting up our
//       camera.
//
//       Geometry is defined in model space. We can move it (translation),
//       rotate it (rotation), or stretch it (scaling) using a world transform.
//       The geometry is then said to be in world space. Next, we need to
//       position the camera, or eye point, somewhere to look at the geometry.
//       Another transform, via the view matrix, is used, to position and
//       rotate our view. With the geometry then in view space, our last
//       transform is the projection transform, which "projects" the 3D scene
//       into our 2D viewport.
//
//       Note that in this tutorial, we are introducing the use of D3DX, which
//       is a set of helper utilities for D3D. In this case, we are using some
//       of D3DX's useful matrix initialization functions. To use D3DX, simply
//       include <d3dx9.h> and link with d3dx9.lib.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <mmsystem.h>
#include <tchar.h>

#include "../../VIOSOWarpBlend/3rdparty/d3dX/Include/d3dx9.h"

#pragma comment(lib, "d3d9.lib" )
#ifdef _M_X64
#pragma comment(lib, "../../VIOSOWarpBlend/3rdparty/d3dX/lib/x64/d3dx9.lib" )
#else
#pragma comment(lib, "../../VIOSOWarpBlend/3rdparty/d3dX/lib/x86/d3dx9.lib" )
#endif

//< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"

LPCTSTR s_configFile = _T("VIOSOWarpBlend.ini");
LPCTSTR s_warpDll = _T("VIOSOWarpBlend");
LPCTSTR s_channel = _T("IG2");
VWB_Warper* warper = NULL;	               // the VIOSO warper
//< end VIOSO API code

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             g_pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // Buffer to hold vertices


// A structure for our custom vertex type
struct CUSTOMVERTEX
{
    FLOAT x, y, z;      // The untransformed, 3D position for the vertex
    DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)




//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    // Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pd3dDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn off culling, so we see the front and back of the triangle
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Turn off D3D lighting, since we are providing our own vertex colors
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

//< start VIOSO API code
	#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
    #include "../../Include/VIOSOWarpBlend.h"

	if( VWB_Create &&
		VWB_Init &&
		VWB_ERROR_NONE == VWB_Create( g_pd3dDevice, s_configFile, s_channel, &warper, 0, NULL ) )
	{
		//VWB_WarpBlendHeaderSet s;
		//if( VWB_vwfInfo && 
		//	VWB_ERROR_NONE == VWB_vwfInfo( warper->calibFile, &s ) )
		//{
			// maybe redecide on calib index...
			if( VWB_Init && 
				VWB_ERROR_NONE == VWB_Init( warper ) )
				return S_OK;
			else if( VWB_Destroy )
				VWB_Destroy( warper );
//		}
	}
	return E_FAIL;
//< end VIOSO API code
}



// Initialize three vertices for rendering a triangle
int g_nTriangles = 16;
CUSTOMVERTEX* g_Vertices; /* [] =
{
    { -1.0f,-1.0f, 3.0f, 0xffff0000, },
    {  1.0f,-1.0f, 3.0f, 0xffff0000, },
    {  0.0f, 1.0f, 3.0f, 0xffff0000, },
    { -3.0f,-1.0f, -1.0f, 0xff00ff00, },
    { -3.0f,-1.0f,  1.0f, 0xff00ff00, },
    { -3.0f, 1.0f,  0.0f, 0xff00ff00, },
    { 3.0f,-1.0f,  1.0f, 0xff0000ff, },
    { 3.0f,-1.0f, -1.0f, 0xff0000ff, },
    { 3.0f, 1.0f,  0.0f, 0xff0000ff, },
    //{ -1.0f,-1.0f, -3.0f, 0xffff00ff, },
    //{  0.0f, 1.0f, -3.0f, 0xffff00ff, },
    //{  1.0f,-1.0f, -3.0f, 0xffff00ff, },
};*/

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Creates the scene geometry
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	g_Vertices = new CUSTOMVERTEX[3 * g_nTriangles];
	for (int i = 0; i != 3 * g_nTriangles; i+= 3)
	{
		float x1 = (float)cos( 2 * D3DX_PI * (i) / g_nTriangles / 3 );
		float z1 = (float)sin( 2 * D3DX_PI * (i) / g_nTriangles / 3 );
		float x2 = (float)cos( 2 * D3DX_PI * (i+1) / g_nTriangles / 3 );
		float z2 = (float)sin( 2 * D3DX_PI * (i+1) / g_nTriangles / 3 );
		float x3 = (float)cos( 2 * D3DX_PI * (i+2) / g_nTriangles / 3 );
		float z3 = (float)sin( 2 * D3DX_PI * (i+2) / g_nTriangles / 3 );
		g_Vertices[i].x = x1;
		g_Vertices[i].y = -0.2f;
		g_Vertices[i].z = z1;
		g_Vertices[i].color = 0xff000000 + i * 0xff / 3 / g_nTriangles + (3 * g_nTriangles - 3 - i ) * 0xff00 / 3 / g_nTriangles;
		g_Vertices[i + 1].x = x3;
		g_Vertices[i + 1].y = -0.2f;
		g_Vertices[i + 1].z = z3;
		g_Vertices[i + 1].color = 0xffff0000 + i * 0xff / 3 / g_nTriangles + (3 * g_nTriangles - 3 - i) * 0xff00 / 3 / g_nTriangles;
		g_Vertices[i + 2].x = x2;
		g_Vertices[i + 2].y = 0.2f;
		g_Vertices[i + 2].z = z2;
		g_Vertices[i + 2].color = 0xff000000 + i * 0xff / 3 / g_nTriangles + (3 * g_nTriangles - 3 - i) * 0xff00 / 3 / g_nTriangles;
	}
    // Create the vertex buffer.
    if( FAILED( g_pd3dDevice->CreateVertexBuffer( sizeof(CUSTOMVERTEX) * 3 * g_nTriangles,
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &g_pVB, NULL ) ) )
    {
        return E_FAIL;
    }

    // Fill the vertex buffer.
    VOID* pVertices;
    if( FAILED( g_pVB->Lock( 0, sizeof(CUSTOMVERTEX) * 3 * g_nTriangles, ( void** )&pVertices, 0 ) ) )
        return E_FAIL;
    memcpy( pVertices, g_Vertices, sizeof(CUSTOMVERTEX) * 3 * g_nTriangles);
    g_pVB->Unlock();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
//< start VIOSO API code
	if( warper && VWB_Destroy )
		VWB_Destroy( warper );
//< end VIOSO API code
    if( g_pVB != NULL )
        g_pVB->Release();

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}



//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{
    // For our world matrix, we will just rotate the object about the y-axis.
	// Set up the rotation matrix to generate 1 full rotation (2*PI radians) 
	// every 1000 ms. To avoid the loss of precision inherent in very high 
	// floating point numbers, the system time is modulated by the rotation 
	// period before conversion to a radian angle.
    D3DXMATRIXA16 matWorld;

	static UINT iTimeS = timeGetTime();// % 10000;
	UINT iTime = (timeGetTime() - iTimeS ) %60000;
	FLOAT fAngle = (FLOAT)sin( D3DX_PI * ( iTime / 30000.0f - 1.0f ) );// / 2.0f;
	fAngle*= D3DX_PI/2; 
	D3DXMatrixRotationY( &matWorld, 0 );
	g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
	fAngle = 0;
	D3DXMATRIX view;
	D3DXMATRIX proj;
    D3DXVECTOR4 vEyePt( 0.0, 0.0f, 0.0f, 1 );
    D3DXVECTOR4 vRot( 0.0f, fAngle, 0.0f, 1 );
	
//< start VIOSO API code
	if( warper && VWB_getViewProj &&
		VWB_ERROR_NONE == VWB_getViewProj( warper, vEyePt, vRot, view, proj ) )
	{
		g_pd3dDevice->SetTransform( D3DTS_VIEW, &view );
		g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &proj );
	}
//< end VIOSO API code
}



//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer to a black color
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 150, 220, 255 ), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();
		HRESULT hr;
        // Render the vertex buffer contents
		hr = g_pd3dDevice->SetVertexShader(NULL);
		hr = g_pd3dDevice->SetPixelShader(NULL);
		hr = g_pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof( CUSTOMVERTEX ) );
		hr = g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, g_nTriangles );

		 
//< start VIOSO API code
		if( warper && VWB_render )
			VWB_render( warper, NULL, VWB_STATEMASK_ALL );
//< end VIOSO API code

		// End the scene
        g_pd3dDevice->EndScene();
    }


    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
{
    UNREFERENCED_PARAMETER( hInst );

    // Register the window class
	WNDCLASSEX wc =
	{
		sizeof( WNDCLASSEX ), 
		CS_CLASSDC, 
		MsgProc, 
		0L, 0L,
		GetModuleHandle( NULL ), 
		NULL, 
		LoadCursor( GetModuleHandle( NULL ), IDC_HAND ), 
		NULL, NULL,
		"D3D Tutorial", NULL
	};
    RegisterClassEx( &wc );

	// Create the application's window
	MONITORINFO mi = { 0 }; mi.cbSize = sizeof(mi);
	static const POINT _p0 = { 1900,0 };
	RECT rc;
	if (GetMonitorInfo(MonitorFromPoint(_p0, MONITOR_DEFAULTTONULL), &mi))
	{
		rc = mi.rcMonitor;
	}
	else
	{
		static const RECT _rc = { 0, 0, 1920, 1200 };
		rc = _rc;
	}
	AdjustWindowRect(&rc, WS_POPUPWINDOW, FALSE);
	HWND hWnd = CreateWindow(
		"D3D Tutorial", "D3D Tutorial 03: Matrices", WS_POPUPWINDOW,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                    Render();
            }
        }
    }

    UnregisterClass( "D3D Tutorial", wc.hInstance );
    return 0;
}



