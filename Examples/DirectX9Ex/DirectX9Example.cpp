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
#include <memory>

#include "resource.h"

//< start VIOSO API code
//#define VWB_USE_DEPRECATED_INIT
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"

LPCTSTR s_configFile = _T("VIOSOWarpBlend.ini");
LPCTSTR s_channel = _T("IG1");
//< end VIOSO API code

D3DXVECTOR4 g_eye( 0, 0, 0, 1 );

// Global Variables:
HINSTANCE hInst;								// current instance
const unsigned int MAX_LOADSTRING = 100;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
std::auto_ptr<DX9Renderer> renderer;              // the d3d renderer
VWB_Warper* warper;	               // the VIOSO warper


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void                DrawFrame();
void                InitialiseRenderer(HWND hWnd);
void                InitialiseWarper(LPDIRECT3DDEVICE9 d3d);


int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DIRECTX9, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (InitInstance (hInstance, nCmdShow))
	{

		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTX9));

		// Main message loop:
		do
		{
			if( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
			{
				if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else
			{
                DrawFrame();
			}
		} while( WM_QUIT != msg.message );

		if( warper )
			VWB_Destroy( warper );
		renderer.reset();

		return (int) msg.wParam;
	}
	return FALSE;
}


//
//  FUNCTION: DrawFrame()
//
//  PURPOSE: Draw a single graphical frame
//
//  COMMENTS:
//
//    This is the main rendering call of the application
//
void DrawFrame()
{
    D3DXMATRIX proj;
    D3DXMATRIX view;
	//< start VIOSO API code
	if (warper && VWB_getViewProj)
		VWB_getViewProj(warper, NULL, NULL, (VWB_float*)&view, (VWB_float*)&proj);
	//< end VIOSO API code

    HRESULT res;
    LPDIRECT3DDEVICE9 d3d = renderer->getDevice();

    res = d3d->BeginScene();

    renderer->draw( view, proj );
//< start VIOSO API code
	if( warper && VWB_render )
		VWB_render( warper, renderer->getOutput(), VWB_STATEMASK_ALL );
//< end VIOSO API code

    res = d3d->EndScene();

    renderer->present();
}

//
//  FUNCTION: InitialiseRenderer()
//
//  PURPOSE: 
//
void InitialiseRenderer(HWND hWnd)
{
    renderer.reset( new DX9Renderer( hWnd ) );
}

//
//  FUNCTION: InitialiseWarper()
//
//  PURPOSE: 
//
void InitialiseWarper(LPDIRECT3DDEVICE9 d3d)
{
//< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "../../Include/VIOSOWarpBlend.h"
	if( VWB_Create )
	{
		VWB_Create( d3d, s_configFile, s_channel, &warper, 0, NULL );
		if( warper && VWB_Init )
			if( VWB_ERROR_NONE == VWB_Init( warper ) )
				return;
	}
	throw -1;
//< end VIOSO API code
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX9));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

	MONITORINFO mi = {0}; mi.cbSize = sizeof(mi);
	static const POINT _p0={0,0};
	RECT rc;
	if( GetMonitorInfo( MonitorFromPoint( _p0, MONITOR_DEFAULTTONULL ), &mi ) )
	{
		rc = mi.rcMonitor;
	}
	else
	{
		static const RECT _rc = { 0, 0, 1920, 1200 };
		rc = _rc;
	}
	AdjustWindowRect( &rc, WS_POPUPWINDOW, FALSE );
    HWND hWnd = CreateWindow( 
		szWindowClass, szTitle, WS_POPUPWINDOW,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL );
    if (!hWnd)
		{
        return FALSE;
		}

    try 
    {
        InitialiseRenderer( hWnd );
        InitialiseWarper( renderer->getDevice() );
	} catch( ... )
	{
		if( renderer.get() )
			renderer.reset();
	}

	if( !renderer.get() )
	{
		::DestroyWindow( hWnd );
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_KEYDOWN:
		switch( wParam )
		{
		case 'A':
			g_eye.x-= 1;
			break;
		case 'D':
			g_eye.x+= 1;
			break;
		case 'W':
			g_eye.y-= 1;
			break;
		case 'S':
			g_eye.y+= 1;
			break;
		case 'Q':
			g_eye.z-= 1;
			break;
		case 'E':
			g_eye.z+= 1;
			break;
		case 27:
			PostQuitMessage(0);
			break;
		default:
			DefWindowProc(hWnd, message, wParam, lParam);
		}
		{
			char s[150];
			sprintf_s( s, "(x=%.1f,y=%.1f,z=%.1f)", g_eye.x, g_eye.y, g_eye.z );
			::SetWindowTextA( hWnd, s );
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
