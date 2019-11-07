//-----------------------------------------------------------------------------
// File:DX11Example.cpp
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
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <tchar.h>
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "resource.h"

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

#define SAFE_RELEASE( x ) if( x ){ x->Release(); x = NULL; }

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
//#pragma comment( lib, "dxerr.lib" )
//#pragma comment( lib, "dxguid.lib" )

//d3d11.lib;d3dcompiler.lib;d3dx11d.lib;d3dx9d.lib;dxerr.lib;dxguid.lib;winmm.lib;comctl32.lib

//< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../VIOSOWarpBlend/Include/VIOSOWarpBlend.h"

LPCTSTR s_configFile = _T("VIOSOWarpBlend.ini");
LPCTSTR s_channel = _T("IGX");
VWB_Warper* warper = NULL;	               // the VIOSO warper
//< end VIOSO API code

int g_nCubes = 9;
int g_nnCubes = 2 * g_nCubes * g_nCubes + 2 * g_nCubes * ( g_nCubes - 2 ) + 2 * ( g_nCubes - 2 ) * ( g_nCubes -2 );
char const g_szShader[] =
"	cbuffer ConstantBuffer : register( b0 )                     \n"
"{																\n"
"	matrix World;												\n"
"	matrix View;												\n"
"	matrix Projection;											\n"
"}																\n"
"																\n"
"//-------------------------------------------------------------\n"
"struct VS_INPUT												\n"
"{																\n"
"    float4 Pos : POSITION;										\n"
"    float4 Color : COLOR;										\n"
"};																\n"
"																\n"
"struct PS_INPUT												\n"
"{																\n"
"    float4 Pos : SV_POSITION;									\n"
"    float4 Color : COLOR;										\n"
"};																\n"
"																\n"
"																\n"
"//-------------------------------------------------------------\n"
"// Vertex Shader												\n"
"//-------------------------------------------------------------\n"
"PS_INPUT VS( VS_INPUT input )									\n"
"{																\n"
"    PS_INPUT output = (PS_INPUT)0;								\n"
"    output.Pos = mul( input.Pos, World );						\n"
"    output.Pos = mul( output.Pos, View );						\n"
"    output.Pos = mul( output.Pos, Projection );				\n"
"    output.Color = input.Color;								\n"
"    															\n"
"    return output;												\n"
"}																\n"
"																\n"
"																\n"
"//-------------------------------------------------------------\n"
"// Pixel Shader												\n"
"//-------------------------------------------------------------\n"
"float4 PS( PS_INPUT input) : SV_Target							\n"
"{																\n"
"    return input.Color;										\n"
"}																\n"
"\n\0";


struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11Texture2D*        g_pDepthStencil = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;
ID3D11VertexShader*     g_pVertexShader = NULL;
ID3D11PixelShader*      g_pPixelShader = NULL;
ID3D11InputLayout*      g_pVertexLayout = NULL;
ID3D11Buffer*           g_pVertexBuffer = NULL;
ID3D11Buffer*           g_pIndexBuffer = NULL;
ID3D11Buffer*           g_pConstantBuffer = NULL;
XMMATRIX                g_World1;
XMMATRIX                g_View;
XMMATRIX                g_Projection;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )_T("directx.ico") );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("TutorialWindowClass");
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )_T("directx.ico") );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
	MONITORINFO mi = {0}; mi.cbSize = sizeof(mi);
	static const POINT _p0={0,0};
	RECT rc;
	if( GetMonitorInfo( MonitorFromPoint( _p0, MONITOR_DEFAULTTONULL ), &mi ) )
	{
		rc = mi.rcMonitor;
	}
	else
	{
		static const RECT _rc = { 0, 0, 800, 450 };
		rc = _rc;
	}
	AdjustWindowRect( &rc, WS_POPUPWINDOW, FALSE );
    g_hWnd = CreateWindow( 
		_T("TutorialWindowClass"), _T("Direct3D 11 Tutorial 5"), WS_POPUPWINDOW,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
   // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

	D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
    hr = D3DCompile( g_szShader, sizeof(g_szShader), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &pVSBlob, NULL );
//hr = CompileShaderFromFile( L"Tutorial05.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    _T("The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file."), _T("Error"), MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
    hr = D3DCompile( g_szShader, sizeof(g_szShader), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &pPSBlob, NULL );
    //hr = CompileShaderFromFile( L"Tutorial05.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    _T("The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file."), _T("Error"), MB_OK );
        return hr;
    }

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Create vertex buffer
	int n = g_nnCubes;
    SimpleVertex* vertices = new SimpleVertex[n*8];
    WORD* indices = new WORD[n*36];
	float sz = 10.0f / ( 3 * g_nCubes - 1 );
	int gg = g_nCubes / 2;
	SimpleVertex* pv = vertices;
	WORD* pi = indices;
	WORD ioffs = 0;
	for( int z = 0; z != g_nCubes; z++ )
	{
		float fz = ( z - gg ) * 3.0f * sz;
		for( int y = 0; y != g_nCubes; y++ )
		{
			float fy = ( y - gg ) * 3.0f * sz;
			for( int x = 0; x != g_nCubes; x++ )
			{
				float fx = ( x - gg ) * 3.0f * sz;
				if( ( 0 == x ) || ( 2*gg == x ) || ( 0 == y ) || ( 2*gg == y ) || ( 0 == z )  || ( 2*gg == z ) )
				{
					SimpleVertex v[8] =  {
						{ XMFLOAT3( fx+sz, fy-sz, fz+sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx+sz, fy+sz, fz+sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx-sz, fy+sz, fz+sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx-sz, fy-sz, fz+sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx+sz, fy-sz, fz-sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx+sz, fy+sz, fz-sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx-sz, fy+sz, fz-sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
						{ XMFLOAT3( fx-sz, fy-sz, fz-sz ), XMFLOAT4( float(x)/(g_nCubes-1), float(y)/(g_nCubes-1), float(z)/(g_nCubes-1), 1.0f ) },
					};
					for( int i = 0; i != 8; i++ )
						*(pv++) = v[i];

					WORD in[36] =
					{
						3,1,0,
						2,1,3,

						0,5,4,
						1,5,0,

						3,4,7,
						0,4,3,

						1,6,5,
						2,6,1,

						2,7,6,
						3,7,2,

						6,4,5,
						7,4,6
					};

					for( int i = 0; i != 36; i++ )
						*(pi++) = in[i] + ioffs;
					ioffs+= 8;
				}
			}
		}
	}

    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * n*8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * n*36;        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pConstantBuffer );
    if( FAILED( hr ) )
        return hr;

	// Turn off culling, so we see the front and back of the triangle
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = true;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	ID3D11RasterizerState* RS = NULL;
	if( SUCCEEDED( g_pd3dDevice->CreateRasterizerState( &rasterDesc, &RS ) ) )
	{
		g_pImmediateContext->RSSetState( RS );
		RS->Release();
	}

    // Initialize the world matrix
	g_World1 = XMMatrixIdentity();
	//g_World1._43 = 10;

    // Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 5.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

    // Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );

//< start VIOSO API code
	#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
	#include "../VIOSOWarpBlend/Include/VIOSOWarpBlend.h"

	if( 
		VWB_Create &&
		VWB_Init &&
		VWB_ERROR_NONE == VWB_Create( g_pd3dDevice, s_configFile, s_channel, &warper, 0, NULL ) &&
		VWB_ERROR_NONE == VWB_Init( warper )
	)
		return S_OK;
	else
		return E_FAIL;
//< end VIOSO API code
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();

	if( VWB_Destroy && warper )
		VWB_Destroy( warper );

    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

		case WM_CHAR:
			if( 27 == wParam )
	            PostQuitMessage( 0 );
            break;
        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
//< start VIOSO API code
	XMFLOAT3 vEyePt( 0.0, 0.0f, 0.0f );
    XMFLOAT3 vRot( 0.0f, 0.0f, 0.0f );
	if( warper && VWB_getViewProj )
		VWB_getViewProj( warper, &vEyePt.x, &vRot.x, (float*)g_View.r, (float*)g_Projection.r );
//< end VIOSO API code


    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.59f, 0.86f, 1.0f, 1.0f }; //red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    //
    // Update variables for the first cube
    //
    ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose( g_World1 );
	cb1.mView = XMMatrixTranspose( g_View );
	cb1.mProjection = XMMatrixTranspose( g_Projection );
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, NULL, &cb1, 0, 0 );

    //
    // Render the first cube
    //
	g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
	g_pImmediateContext->DrawIndexed( g_nnCubes * 36, 0, 0 );


//< start VIOSO API code
	if( warper && VWB_render )
		VWB_render( warper, NULL, VWB_STATEMASK_STANDARD );
//< end VIOSO API code

	//
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 1, 0 );
	//Sleep(100);
}