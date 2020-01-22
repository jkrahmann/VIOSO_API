#include <windows.h>		// Header File For Windows
#include <stdio.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "../../VIOSOWarpBlend/GL/GLext.h"

#define USE_VIOSO_API
const int c_numTri = 30;
const GLfloat c_rad = 3;

#ifdef USE_VIOSO_API
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"

LPCTSTR s_configFile = _T("VIOSOWarpBlendGL.ini");
LPCTSTR s_channel = _T("IG1");
VWB_Warper* pWarper = NULL;
#endif //USE_VIOSO_API

#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
typedef char GLchar;

GLuint iProg = -1;				// the shader program
GLuint iVert = -1;				// the vertex shader program
GLuint iFrag = -1;				// the fragment shader
GLuint iVAData = -1;			// the data buffer
GLuint iVAPos = -1;				// the vertex array for position
GLuint iVACol = -1;				// the vertex array for color
GLuint iVATex = -1;				// the vertex array for texture
GLuint locVAPos = -1;			// the location of the position input
GLuint locVACol = -1;			// the location of the color input
GLuint locVATex = -1;			// the location of the texture input
GLuint locMatViewProj = -1;     // the location of the view/proj matrix

__declspec(align(4)) struct Vertex
{
	GLfloat x, y, z;
	GLfloat r, g, b, a;
	GLfloat u, v;
};
std::vector<Vertex> vertices;

GLchar const* pszShaders[] = {
"#version 330 core													\n"
"uniform mat4 matViewProj;											\n"
"layout( location = 0 ) in vec3 in_position;						\n"
"layout( location = 1 ) in vec4 in_color;							\n"
//"layout( location = 2 ) in vec2 in_tex;								\n"
"out vec4 vtx_color;												\n"
"																	\n"
"void main(void) {													\n"
"	gl_Position = vec4(in_position.xyz, 1.0);						\n"
"	gl_Position*= matViewProj;										\n"
"	gl_Position.x/= gl_Position.w;									\n"
"	gl_Position.y/= gl_Position.w;									\n"
"	gl_Position.z/= gl_Position.w;									\n"
"	gl_Position.w/= 1.0;											\n"
//"	vtx_color = vec4(1.0,0.9,0.4,1.0);											\n"
"	vtx_color = in_color;											\n"
"}																	\n",
"#version 330 core														\n"
"precision highp float; // Video card drivers require this line to function properly\n"
"in vec4 vtx_color;													\n"
"out vec4 out_color;												\n"
"void main()														\n"
"{																	\n"
//"	out_color = vec4(1.0,1.0,1.0,1.0);								\n"
"	out_color = vtx_color;											\n"
"}																	\n"
};

HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=false;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	changeFSmode = false; // Fullscreen mode needs change

GLvoid ReSizeGLScene(GLsizei width, GLsizei height);		// Resize And Initialize The GL Window

#ifdef WIN32
#include <crtdbg.h>
#endif

inline void logStr(const char* str)
{
	if( NULL != VWB__logString )
		VWB__logString( 1, str );
	else
	{
		FILE* f;
		{
			if( NOERROR == fopen_s( &f, "VIOSOWarpBlend.log", "a" ) )
			{
				fprintf( f, str );
				fclose( f );
			}
		}
	}
}

void APIENTRY glLog(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void* userParam )
{
	char str[2047];
	char const* szType = nullptr;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		szType = "FATAL";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		szType = "Deprecated";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		szType = "Undefined";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		szType = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		szType = "Performance";
		break;
	case GL_DEBUG_TYPE_OTHER:
		szType = "Other";
		break;
	}
	sprintf_s(str, "GLDEBUG: %s %s: %s\n", szType, GL_DEBUG_SEVERITY_LOW == severity ? "note" : (GL_DEBUG_SEVERITY_MEDIUM == severity ? "warning" : "ERROR"), message);
	logStr(str);
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
#ifdef USE_VIOSO_API
	if( pWarper && VWB_Destroy )
		VWB_Destroy( pWarper );
#endif //def USE_VIOSO_API

	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	GLenum err = glGetError();

	#define GL_EXT_INITIALIZE
	#include "../../VIOSOWarpBlend/GL/GLext.h"

#if defined( _DEBUG)
	glDebugMessageCallback(&glLog, nullptr);
	glEnable(GL_DEBUG_OUTPUT);							// Enables Debug output
	glDebugMessageCallback(&glLog, nullptr);
#endif

	RECT rWnd;
	GetWindowRect( hWnd, &rWnd );
	DWORD dwStyle = GetWindowLong( hWnd, GWL_STYLE );
	DWORD dwExStyle = GetWindowLong( hWnd, GWL_EXSTYLE );
	char const* szVer = (const char*)glGetString(GL_VERSION);
	HWND newHwnd; 
	if (!(newHwnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								szVer,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								rWnd.left, rWnd.top,								// Window Position
								rWnd.right-rWnd.left,	// Calculate Window Width
								rWnd.bottom-rWnd.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}
	HDC newHDC = GetDC( newHwnd );
	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	int pixelFormatID; UINT numFormats;
	BOOL status = wglChoosePixelFormatARB(newHDC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
 
	if (status == false || numFormats == 0)
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(newHDC, pixelFormatID, sizeof(pfd), &pfd);

	if(!SetPixelFormat(newHDC,pixelFormatID,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	const int attribList[] = { 
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3, 
		WGL_CONTEXT_MINOR_VERSION_ARB, 2, 
		0, 0 };

	HGLRC thRC;
	if( !(thRC = wglCreateContextAttribsARB( newHDC, 0, attribList ) ) )
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	wglMakeCurrent( hDC, NULL );
	wglDeleteContext( hRC );
	ReleaseDC( hWnd, hDC );
	DestroyWindow( hWnd );
	hWnd = newHwnd;
	hDC = newHDC;
	hRC = thRC;
	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

#if defined( _DEBUG)
	glDebugMessageCallback(&glLog, nullptr);
	glEnable(GL_DEBUG_OUTPUT);							// Enables Debug output
	glDebugMessageCallback(&glLog, nullptr);
#endif

	//glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	// vertex shader
	iVert = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( iVert, 1, &pszShaders[0], NULL );
	glCompileShader( iVert );
	char log[2000] = {0};
	glGetShaderInfoLog( iVert, 2000, NULL, log );
	if( log[0] )
	{
		logStr(log);
		return FALSE;
	}

	// fragment shader
	iFrag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(iFrag, 1, &pszShaders[1], NULL);
	glCompileShader(iFrag);
	glGetShaderInfoLog(iFrag, 2000, NULL, log);
	if (log[0])
	{
		logStr(log);
		return FALSE;
	}
	err = glGetError();

	iProg = glCreateProgram();
	glAttachShader(iProg, iVert);
	glAttachShader(iProg, iFrag);
	glLinkProgram(iProg);
	err = glGetError();
	if (GL_NO_ERROR != err)
	{
		logStr("Failed to link shader program");
		return FALSE;
	}
	GLint isLinked = 0;
	glGetProgramiv(iProg, GL_LINK_STATUS, &isLinked);
	if(isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(iProg, GL_INFO_LOG_LENGTH, &maxLength);

		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(iProg, maxLength, &maxLength, &infoLog[0]);

		//The program is useless now. So delete it.
		glDeleteProgram(iProg);
		iProg = -1;

		char log[1024];
		sprintf_s( log, "ERROR: %d at glLinkProgram:\n%s\n", err, &infoLog[0] );
		logStr( log );
		return FALSE;
	}

	locMatViewProj = glGetUniformLocation(iProg, "matViewProj");
	locVAPos = glGetAttribLocation(iProg, "in_position");
	locVACol = glGetAttribLocation(iProg, "in_color");
	locVATex = glGetAttribLocation(iProg, "in_tex");
	glUseProgram(iProg);

	err = glGetError();
	if (GL_NO_ERROR != err)
	{
		logStr("Failed to create vertex position buffer");
		return FALSE;
	}

	vertices.clear();
	GLfloat b = 0.5f * GLfloat(M_PI) / c_numTri;
	for (int i = 0; i != c_numTri; i++)
	{
		GLfloat a = 4.0f * b * i;
		Vertex v = {0};

		v.x = -c_rad * sinf(a-b);
		v.y = 1;
		v.z = -c_rad * cosf(a-b);
		v.r = 1;
		v.g = 0;
		v.b = 0;
		v.a = 1;
		v.u = 0;
		v.v = 1;
		vertices.push_back( v );

		v.x = -c_rad * sinf(a+b);
		v.y = 1;
		v.z = -c_rad * cosf(a+b);
		v.r = 0;
		v.g = 1;
		v.b = 0;
		v.a = 1;
		v.u = 1;
		v.v = 1;

		GLfloat c = 0.5f * ( v.x + vertices.back().x );
		GLfloat d = 0.5f * ( v.z + vertices.back().z );
		vertices.push_back( v );

		v.x = c;
		v.y = -1;
		v.z = d;
		v.r = 0;
		v.g = 0;
		v.b = 1;
		v.a = 1;
		v.u = 0.5f;
		v.v = 1;
		vertices.push_back( v );

	}

	glGenBuffers(1, &iVAData);
	glGenVertexArrays(1, &iVAPos);
	glBindVertexArray(iVAPos);
	glGenVertexArrays(1, &iVACol);
	glBindVertexArray(iVACol);
	glGenVertexArrays(1, &iVATex);
	glBindVertexArray(iVATex);
	glBindBuffer(GL_ARRAY_BUFFER, iVAData);
	glBufferData(GL_ARRAY_BUFFER, (int)(sizeof(Vertex) * vertices.size()), &vertices[0], GL_STATIC_DRAW);

	err = glGetError();
	if (GL_NO_ERROR != err)
	{
		logStr("Failed to set vertex position buffer");
		return FALSE;
	}
	return TRUE;										// Initialization Went OK
}

bool DrawGLScene(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
#ifdef USE_VIOSO_API
	//< start VIOSO API code
	GLfloat M[16] = {0};
	GLfloat view[16], proj[16];
	GLfloat eye[3] = {0,0,0};
	GLfloat rot[3] = {0,0,0};

	if( pWarper && VWB_getViewProj )
	{
		VWB_getViewProj( pWarper, eye, rot, view, proj );
		for( int i = 0; i != 4; i++ )
			for( int j = 0; j != 4; j++ )
			{
				M[i*4+j] = view[i*4] * proj[j];
				for( int k = 1; k != 4; k++ )
					M[i*4+j]+= view[i*4+k] * proj[k*4+j];
			}
	}
	//< end VIOSO API code
#else
    GLfloat M[16] =	{
        2*n/(r-l),			0,     0,				       0,
                0,	2*n/(t-b),     0,			           0,
(r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1,
                0,			       0,  2 * f*n / (f - n),  0
	};
#endif //def USE_VIOSO_API


	/* Set background colour to light blue sky color */
	glClearColor(0.6f, 0.8f, 1, 1.0f);

	/* Clear background with BLACK colour */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(locMatViewProj, 1, GL_TRUE, M);

	glBindVertexArray(iVAPos);
	glBindBuffer(GL_ARRAY_BUFFER, iVAData);

	glEnableVertexAttribArray(locVAPos);
	glVertexAttribPointer(locVAPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x)); // position

	if (-1 != locVACol)
	{
		glEnableVertexAttribArray(locVACol);
		glVertexAttribPointer(locVACol, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r)); // color
	}

	if (-1 != locVATex)
	{
		glEnableVertexAttribArray(locVACol);
		glVertexAttribPointer(locVACol, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u)); // color
	}
	/* Actually draw the triangle, giving the number of vertices provided by invoke glDrawArrays
	   while telling that our data is a triangle and we want to draw 0-3 vertexes
	*/
	glDrawArrays(GL_TRIANGLES, 0, (int)vertices.size() );
	glDisableVertexAttribArray(locVAPos);
	if (-1 != locVACol)
	{
		glDisableVertexAttribArray(locVACol);
	}
	if (-1 != locVATex)
	{
		glDisableVertexAttribArray(locVATex);
	}
#ifdef USE_VIOSO_API
	//< start VIOSO API code
	if( pWarper && VWB_render )
	{
		VWB_render( pWarper, VWB_UNDEFINED_GL_TEXTURE, VWB_STATEMASK_DEFAULT|VWB_STATEMASK_PIXEL_SHADER );
	}
	//< end VIOSO API code
#endif //def USE_VIOSO_API

	return true;
}

LRESULT CALLBACK WndProc(HWND	hWnd,			// Handle For This Window
	UINT	uMsg,			// Message For This Window
	WPARAM	wParam,			// Additional Message Information
	LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
	case WM_ACTIVATE:							// Watch For Window Activate Message
	{
		// LoWord Can Be WA_INACTIVE, WA_ACTIVE, WA_CLICKACTIVE,
		// The High-Order Word Specifies The Minimized State Of The Window Being Activated Or Deactivated.
		// A NonZero Value Indicates The Window Is Minimized.
		if ((LOWORD(wParam) != WA_INACTIVE) && !((BOOL)HIWORD(wParam)))
			active = TRUE;						// Program Is Active
		else
			active = FALSE;						// Program Is No Longer Active

		return 0;								// Return To The Message Loop
	}

	case WM_SYSCOMMAND:							// Intercept System Commands
	{
		switch (wParam)							// Check System Calls
		{
		case SC_SCREENSAVE:					// Screensaver Trying To Start?
		case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
			return 0;							// Prevent From Happening
		}
		break;									// Exit
	}

	case WM_CLOSE:								// Did We Receive A Close Message?
	{
		PostQuitMessage(0);						// Send A Quit Message
		return 0;								// Jump Back
	}

	case WM_KEYDOWN:							// Did We Receive Key?
	{
		if (VK_ESCAPE == wParam)
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}
		else if (VK_F1 == wParam)
		{
			changeFSmode = true;						// Send A Quit Message
			return 0;								// Jump Back
		}
	}

	case WM_SIZE:								// Resize The OpenGL Window
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWord=Width, HiWord=Height
		return 0;								// Jump Back
	}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left = (long)0;			// Set Left Value To 0
	WindowRect.right = (long)width;		// Set Right Value To Requested Width
	WindowRect.top = (long)0;				// Set Top Value To 0
	WindowRect.bottom = (long)height;		// Set Bottom Value To Requested Height

	fullscreen = fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance = GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc = (WNDPROC)WndProc;					// WndProc Handles Messages
	wc.cbClsExtra = 0;									// No Extra Window Data
	wc.cbWndExtra = 0;									// No Extra Window Data
	wc.hInstance = hInstance;							// Set The Instance
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground = NULL;									// No Background Required For GL
	wc.lpszMenuName = NULL;									// We Don't Want A Menu
	wc.lpszClassName = "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}

	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth = width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight = height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel = bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL, "The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?", "NeHe GL", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				fullscreen = FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL, "Program Will Now Close.", "ERROR", MB_OK | MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle = WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle = WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd = CreateWindowEx(dwExStyle,							// Extended Style For The Window
		"OpenGL",							// Class Name
		title,								// Window Title
		dwStyle |							// Defined Window Style
		WS_CLIPSIBLINGS |					// Required Window Style
		WS_CLIPCHILDREN,					// Required Window Style
		0, 0,								// Window Position
		WindowRect.right - WindowRect.left,	// Calculate Window Width
		WindowRect.bottom - WindowRect.top,	// Calculate Window Height
		NULL,								// No Parent Window
		NULL,								// No Menu
		hInstance,							// Instance
		NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd =				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		(BYTE)bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		24,											// 16Bit Z-Buffer (Depth Buffer)  
		8,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(hDC = GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}


	if (!(hRC = wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Create A GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!wglMakeCurrent(hDC, hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Can't Activate The GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd, SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen
	return TRUE;									// Success
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	// Ask The User Which Screen Mode They Prefer
	//if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
		fullscreen=FALSE;							// Windowed Mode
	}

	// Create Our OpenGL Window
	if (!CreateGLWindow("NeHe's Solid Object Tutorial",1280,800,32,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

#ifdef USE_VIOSO_API
	#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
	#include "../../Include/VIOSOWarpBlend.h"

	if( NULL == VWB_Create || 
		VWB_ERROR_NONE != VWB_Create( NULL, s_configFile, s_channel, &pWarper, 1, NULL ) )
		return FALSE;
	// maybe check and modify settings here
	if( NULL == pWarper ||
		NULL == VWB_Init ||
		VWB_ERROR_NONE != VWB_Init(pWarper) )
		return FALSE;
#endif //def USE_VIOSO_API

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (active && !PeekMessage(&msg,NULL,0,0,PM_REMOVE)) // we are active, Is There A Message Waiting?
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if (!DrawGLScene(-0.5,0.5,-0.5,0.5,2,200))	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
			}
		}
		else if (
			    !active && GetMessage(&msg,NULL,0,0) ||
				WM_QUIT != msg.message
			) // do messages
		{
			TranslateMessage(&msg);				// Translate The Message
			DispatchMessage(&msg);				// Dispatch The Message
			if (changeFSmode)						// Is F1 Being Pressed?
			{
				changeFSmode = false;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				if (!CreateGLWindow("NeHe's Solid Object Tutorial",640,480,16,fullscreen))
				{
					return 0;						// Quit If Window Was Not Created
				}
			}
		}
		else
		{
			done = TRUE;
		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return int(msg.wParam);							// Exit The Program
}
