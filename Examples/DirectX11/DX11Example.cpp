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
#include <memory>
#include <vector>
#include <string>
#include <exception>

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

//#define USE_VIOSO
#ifdef USE_VIOSO
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"

LPCTSTR s_configFile = _T("VIOSOWarpBlend.ini");
#endif //def USE_VIOSO

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
XMMATRIX                g_World1;

class OutWnd
{
	#ifdef USE_VIOSO
	VWB_Warper*				m_warper = NULL;	               // the VIOSO warper
	#endif //def USE_VIOSO
	HWND                    m_hWnd = NULL;
	D3D_DRIVER_TYPE         m_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	ID3D11Device*           m_pd3dDevice = NULL;
	ID3D11DeviceContext*    m_pImmediateContext = NULL;
	IDXGISwapChain*         m_pSwapChain = NULL;
	ID3D11RenderTargetView* m_pRenderTargetView = NULL;
	ID3D11Texture2D*        m_pDepthStencil = NULL;
	ID3D11DepthStencilView* m_pDepthStencilView = NULL;
	ID3D11VertexShader*     m_pVertexShader = NULL;
	ID3D11PixelShader*      m_pPixelShader = NULL;
	ID3D11InputLayout*      m_pVertexLayout = NULL;
	ID3D11Buffer*           m_pVertexBuffer = NULL;
	ID3D11Buffer*           m_pIndexBuffer = NULL;
	ID3D11Buffer*           m_pConstantBuffer = NULL;
	XMMATRIX                m_View;
	XMMATRIX                m_Projection;
private:
	OutWnd() {}
	OutWnd( OutWnd const& other ) { UNREFERENCED_PARAMETER( other ); }
public:
	OutWnd( LPCRECT pPos, std::string const& name );
	virtual ~OutWnd();
	HRESULT Render();
};

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );

BOOL CALLBACK enFnc(
	HMONITOR hMon,
	HDC hDC,
	LPRECT pRect,
	LPARAM pData
)
{
	UNREFERENCED_PARAMETER( hMon );
	UNREFERENCED_PARAMETER( hDC );
	// we wanna skip main window, as we only play on extended windows displays
#ifndef _DEBUG
	if( 0 == pRect->left && 0 == pRect->top )
		return TRUE;
#endif /ndef _DEBUG
	std::vector< std::shared_ptr< OutWnd > >& wndList = *( std::vector< std::shared_ptr< OutWnd > >* )pData;
	std::string s( "Display" );
	s += std::to_string( wndList.size() + 1 );
	wndList.push_back( std::shared_ptr< OutWnd >( new OutWnd( pRect, s ) ) );
	return TRUE;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );
	UNREFERENCED_PARAMETER( nCmdShow );

	std::vector< std::shared_ptr< OutWnd > > wndList;

	g_hInst = hInstance;

#ifdef USE_VIOSO
	#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
	#include "../../Include/VIOSOWarpBlend.h"
#endif //def USE_VIOSO

	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance, (LPCTSTR)_T( "directx.ico" ) );
	wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T( "TutorialWindowClass" );
	wcex.hIconSm = LoadIcon( wcex.hInstance, (LPCTSTR)_T( "directx.ico" ) );
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	EnumDisplayMonitors( NULL, NULL, &enFnc, (LPARAM)&wndList );

	// Initialize the world matrix
	g_World1 = XMMatrixIdentity();
	//g_World1._43 = 10;

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
			for( auto it = wndList.begin(); it != wndList.end(); it++ )
			{
				( *it )->Render();
			}
        }
    }

	// this will destroy all windows
	wndList.clear();

    return ( int )msg.wParam;
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
HRESULT OutWnd::Render()
{
#ifdef USE_VIOSO
	XMFLOAT3 vEyePt( 0.0, 0.0f, 0.0f );
    XMFLOAT3 vRot( 0.0f, 0.0f, 0.0f );
	if( m_warper && VWB_getViewProj )
		VWB_getViewProj( m_warper, &vEyePt.x, &vRot.x, (float*)m_View.r, (float*)m_Projection.r );
#else
	m_View = XMMatrixIdentity();
	m_Projection = XMMatrixPerspectiveOffCenterLH( -1, 1, -1, 1, 0.25f, 1024.25f);
#endif //def USE_VIOSO


    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.59f, 0.86f, 1.0f, 1.0f }; //red, green, blue, alpha
    m_pImmediateContext->ClearRenderTargetView( m_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    m_pImmediateContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    //
    // Update variables for the first cube
    //
    ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose( g_World1 );
	cb1.mView = XMMatrixTranspose( m_View );
	cb1.mProjection = XMMatrixTranspose( m_Projection );
	m_pImmediateContext->UpdateSubresource( m_pConstantBuffer, 0, NULL, &cb1, 0, 0 );

    //
    // Render the first cube
    //
	m_pImmediateContext->VSSetShader( m_pVertexShader, NULL, 0 );
	m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
	m_pImmediateContext->PSSetShader( m_pPixelShader, NULL, 0 );
	m_pImmediateContext->DrawIndexed( g_nCubes * 36, 0, 0 );

	#ifdef USE_VIOSO
	if( m_warper && VWB_render )
		VWB_render( m_warper, NULL, VWB_STATEMASK_STANDARD );
	#endif //def USE_VIOSO

	//
    // Present our back buffer to our front buffer
    //
    m_pSwapChain->Present( 1, 0 );
	//Sleep(100);

	return S_OK;
}

OutWnd::OutWnd( LPCRECT pPos, std::string const& name ) :
#ifdef USE_VIOSO
  m_warper( NULL )	,               // the VIOSO warper
#endif //def USE_VIOSO
  m_hWnd( NULL )
, m_driverType( D3D_DRIVER_TYPE_NULL )
, m_featureLevel( D3D_FEATURE_LEVEL_11_0 )
, m_pd3dDevice( NULL )
, m_pImmediateContext( NULL )
, m_pSwapChain( NULL )
, m_pRenderTargetView( NULL )
, m_pDepthStencil( NULL )
, m_pDepthStencilView( NULL )
, m_pVertexShader( NULL )
, m_pPixelShader( NULL )
, m_pVertexLayout( NULL )
, m_pVertexBuffer( NULL )
, m_pIndexBuffer( NULL )
, m_pConstantBuffer( NULL )
, m_View( XMMatrixIdentity()  )
, m_Projection( XMMatrixIdentity() )
{
	// Create window
	MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );
	static const POINT _p0 = { 0,0 };
	RECT rc = *pPos;
	AdjustWindowRect( &rc, WS_POPUPWINDOW, FALSE );
	m_hWnd = CreateWindow(
		"TutorialWindowClass", name.c_str(), WS_POPUPWINDOW,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, g_hInst, NULL );
	if( !m_hWnd )
		throw std::exception( "CreateWindow failed" );

	ShowWindow( m_hWnd, SW_SHOW );

	HRESULT hr = S_OK;

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
	sd.OutputWindow = m_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		m_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
											D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext );
		if( SUCCEEDED( hr ) )
			break;
	}
	if( FAILED( hr ) )
		throw std::exception( "D3D11CreateDeviceAndSwapChain failed" );

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
	if( FAILED( hr ) )
		throw std::exception( "GetBuffer failed" );

	hr = m_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) )
		throw std::exception( "CreateRenderTargetView failed" );

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory( &descDepth, sizeof( descDepth ) );
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
	hr = m_pd3dDevice->CreateTexture2D( &descDepth, NULL, &m_pDepthStencil );
	if( FAILED( hr ) )
		throw std::exception( "CreateTexture2D failed" );

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof( descDSV ) );
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = m_pd3dDevice->CreateDepthStencilView( m_pDepthStencil, &descDSV, &m_pDepthStencilView );
	if( FAILED( hr ) )
		throw std::exception( "CreateDepthStencilView failed" );

	m_pImmediateContext->OMSetRenderTargets( 1, &m_pRenderTargetView, m_pDepthStencilView );

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pImmediateContext->RSSetViewports( 1, &vp );

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = D3DCompile( g_szShader, sizeof( g_szShader ), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &pVSBlob, NULL );
	//hr = CompileShaderFromFile( L"Tutorial05.fx", "VS", "vs_4_0", &pVSBlob );
	if( FAILED( hr ) )
	{
		throw std::exception( "D3DCompile failed" );
	}

	// Create the vertex shader
	hr = m_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pVertexShader );
	if( FAILED( hr ) )
	{
		pVSBlob->Release();
		throw std::exception( "CreateVertexShader failed" );
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

	// Create the input layout
	hr = m_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
										  pVSBlob->GetBufferSize(), &m_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
		throw std::exception( "CreateInputLayout failed" );

	// Set the input layout
	m_pImmediateContext->IASetInputLayout( m_pVertexLayout );

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = D3DCompile( g_szShader, sizeof( g_szShader ), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &pPSBlob, NULL );
	//hr = CompileShaderFromFile( L"Tutorial05.fx", "PS", "ps_4_0", &pPSBlob );
	if( FAILED( hr ) )
	{
		throw std::exception( "D3DCompile failed" );
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pPixelShader );
	pPSBlob->Release();
	if( FAILED( hr ) )
		throw std::exception( "D3DCompile failed" );

	// Create vertex buffer
	int n = g_nnCubes;
	SimpleVertex* vertices = new SimpleVertex[n * 8];
	WORD* indices = new WORD[n * 36];
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
				if( ( 0 == x ) || ( 2 * gg == x ) || ( 0 == y ) || ( 2 * gg == y ) || ( 0 == z ) || ( 2 * gg == z ) )
				{
					SimpleVertex v[8] = {
						{ XMFLOAT3( fx + sz, fy - sz, fz + sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx + sz, fy + sz, fz + sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx - sz, fy + sz, fz + sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx - sz, fy - sz, fz + sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx + sz, fy - sz, fz - sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx + sz, fy + sz, fz - sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx - sz, fy + sz, fz - sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
						{ XMFLOAT3( fx - sz, fy - sz, fz - sz ), XMFLOAT4( float( x ) / ( g_nCubes - 1 ), float( y ) / ( g_nCubes - 1 ), float( z ) / ( g_nCubes - 1 ), 1.0f ) },
					};
					for( int i = 0; i != 8; i++ )
						*( pv++ ) = v[i];

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
						*( pi++ ) = in[i] + ioffs;
					ioffs += 8;
				}
			}
		}
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( SimpleVertex ) * n * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof( InitData ) );
	InitData.pSysMem = vertices;
	hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pVertexBuffer );
	if( FAILED( hr ) )
		throw std::exception( "CreateBuffer (vertex) failed" );

	// Set vertex buffer
	UINT stride = sizeof( SimpleVertex );
	UINT offset = 0;
	m_pImmediateContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * n * 36;        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pIndexBuffer );
	if( FAILED( hr ) )
		throw std::exception( "CreateBuffer (index) failed" );

	// Set index buffer
	m_pImmediateContext->IASetIndexBuffer( m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// Set primitive topology
	m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// Create the constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( ConstantBuffer );
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer( &bd, NULL, &m_pConstantBuffer );
	if( FAILED( hr ) )
		throw std::exception( "CreateBuffer (constants) failed" );

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
	if( SUCCEEDED( m_pd3dDevice->CreateRasterizerState( &rasterDesc, &RS ) ) )
	{
		m_pImmediateContext->RSSetState( RS );
		RS->Release();
	}

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 5.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	m_View = XMMatrixLookAtLH( Eye, At, Up );

	// Initialize the projection matrix
	m_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f );

	#ifdef USE_VIOSO
	if( !(
		VWB_Create &&
		VWB_Init &&
		VWB_ERROR_NONE == VWB_Create( m_pd3dDevice, s_configFile, name.c_str(), &m_warper, 0, NULL ) &&
		VWB_ERROR_NONE == VWB_Init( m_warper )
		) )
		throw std::exception( "Warper initialization failed" );
	#endif //def USE_VIOSO
}

OutWnd::~OutWnd()
{
	if( m_pImmediateContext ) m_pImmediateContext->ClearState();

	if( m_pConstantBuffer ) m_pConstantBuffer->Release();
	if( m_pVertexBuffer ) m_pVertexBuffer->Release();
	if( m_pIndexBuffer ) m_pIndexBuffer->Release();
	if( m_pVertexLayout ) m_pVertexLayout->Release();
	if( m_pVertexShader ) m_pVertexShader->Release();
	if( m_pPixelShader ) m_pPixelShader->Release();
	if( m_pDepthStencil ) m_pDepthStencil->Release();
	if( m_pDepthStencilView ) m_pDepthStencilView->Release();
	if( m_pRenderTargetView ) m_pRenderTargetView->Release();
	if( m_pSwapChain ) m_pSwapChain->Release();
	if( m_pImmediateContext ) m_pImmediateContext->Release();

	#ifdef USE_VIOSO
	if( VWB_Destroy && m_warper )
		VWB_Destroy( m_warper );
	#endif //def USE_VIOSO

	if( m_pd3dDevice ) m_pd3dDevice->Release();

}