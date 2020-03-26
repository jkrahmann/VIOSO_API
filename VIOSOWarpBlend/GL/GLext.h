// usage:
// #include GLext in your header
// to implement symbols #define GL_EXT_INITIALIZE and #include GLext on top of your code file, below all header includes
// to initialize, #define GL_EXT_INITIALIZE and #include GLext.h in code after making your context current
// to test, if all functions are initialized, #define GL_EXT_TEST and #include GLext inside a if( ) statement
// you might also #define GL_EXT_EXP to do something different.

	//typedef enum MONITOR_DPI_TYPE_L {
	//  MDT_EFFECTIVE_DPI_L,
	//  MDT_ANGULAR_DPI_L,
	//  MDT_RAW_DPI_L,
	//  MDT_DEFAULT_L
	//};
	//typedef BOOL (WINAPI *pfnAdjustWindowRectExForDpi)(LPRECT lpRect,DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi );
	//pfnAdjustWindowRectExForDpi AdjustWindowRectExForDpi = reinterpret_cast<pfnAdjustWindowRectExForDpi>( GetProcAddress( ::GetModuleHandleA( "user32" ), "AdjustWindowRectExForDpi" ) );
	//typedef HRESULT (WINAPI *pfnGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE_L dpiType, UINT *dpiX, UINT *dpiY );
	//pfnGetDpiForMonitor GetDpiForMonitor = reinterpret_cast<pfnGetDpiForMonitor>( GetProcAddress( ::GetModuleHandleA( "shcore" ), "GetDpiForMonitor" ) );
	//UINT dpiX,dpiY;
	//POINT p = {0,0};
	//if( AdjustWindowRectExForDpi && GetDpiForMonitor && GetDpiForMonitor( MonitorFromPoint( p, MONITOR_DEFAULTTOPRIMARY ), MDT_DEFAULT_L, &dpiX, &dpiY ) )
	//{
	//	AdjustWindowRectExForDpi(&WindowRect, dwStyle, FALSE, dwExStyle, dpiX );
	//}
	//else

#if !defined( GL_EXT_EXP )
	#if defined( GL_EXT_IMPLEMENT )
		#define GL_EXT_EXP( ret, name, args ) pfn_##name name = NULL;
		#undef GL_EXT_IMPLEMENT
	#elif defined( GL_EXT_DEFINE_AND_IMPLEMENT )
		#define GL_EXT_EXP( ret, name, args ) typedef ret ( __stdcall *pfn_##name)args;pfn_##name name = NULL;
		#undef GL_EXT_DEFINE_AND_IMPLEMENT
	#elif defined( GL_EXT_INITIALIZE )
        #if defined(WIN32)
            #define GL_EXT_EXP( ret, name, args ) name = (pfn_##name)::wglGetProcAddress( #name );
        #else
            // use Glew
        #endif
		#undef GL_EXT_INITIALIZE
	#elif defined( GL_EXT_TEST )
		1
		#define GL_EXT_EXP( ret, name, args ) && NULL != name
		#undef GL_EXT_TEST
	#elif defined( GL_EXT_TEST_VERBOSE )
		1
	#define GL_EXT_EXP( ret, name, args ) && ( NULL != name || logStr( 0, "ERROR: GLext function " #name " not loaded.\n" ) )
		#undef GL_EXT_TEST_VERBOSE
	#else
		#define GL_EXT_EXP( ret, name, args ) typedef ret ( __stdcall *pfn_##name)args;extern pfn_##name name;
	#endif
#endif //!defined( GL_EXT_EXP )

#if !defined( GLEXT_DEFINES_DEFINED )
#define GLEXT_DEFINES_DEFINED

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif WIN32
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#if defined(WIN32)
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )

typedef char GLchar;
typedef int GLsizeiptrARB;
typedef void (WINAPI  *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam );
#define GL_POINTS			                    0x0000
#define GL_LINES								0x0001
#define GL_LINE_LOOP							0x0002
#define GL_LINE_STRIP						    0x0003
#define GL_TRIANGLES							0x0004
#define GL_TRIANGLE_STRIP						0x0005
#define GL_TRIANGLE_FAN							0x0006
#define GL_ZERO									0
#define GL_ONE									1
#define GL_FUNC_ADD								0x8006
#define GL_BLEND_EQUATION						0x8009
#define GL_BLEND_EQUATION_RGB					0x8009
#define GL_BLEND_EQUATION_ALPHA					0x883D
#define GL_FUNC_SUBTRACT						0x800A
#define GL_FUNC_REVERSE_SUBTRACT				0x800B
#define GL_BLEND_DST_RGB						0x80C8
#define GL_BLEND_SRC_RGB						0x80C9
#define GL_BLEND_DST_ALPHA						0x80CA
#define GL_BLEND_SRC_ALPHA						0x80CB
#define GL_CONSTANT_COLOR						0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR				0x8002
#define GL_CONSTANT_ALPHA						0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA				0x8004
#define GL_BLEND_COLOR							0x8005
#define GL_ARRAY_BUFFER							0x8892
#define GL_ELEMENT_ARRAY_BUFFER					0x8893
#define GL_ARRAY_BUFFER_BINDING					0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING			0x8895
#define GL_STREAM_DRAW							0x88E0
#define GL_STATIC_DRAW							0x88E4
#define GL_DYNAMIC_DRAW							0x88E8
#define GL_BUFFER_SIZE							0x8764
#define GL_BUFFER_USAGE							0x8765
#define GL_CURRENT_VERTEX_ATTRIB				0x8626
#define GL_FRONT								0x0404
#define GL_BACK									0x0405
#define GL_FRONT_AND_BACK						0x0408
#define GL_CLAMP_TO_BORDER						0x812D
#define GL_CLAMP_TO_EDGE						0x812F
#define GL_R8									0x8229
#define GL_R16									0x822A
#define GL_RG8									0x822B
#define GL_RG16									0x822C
#define GL_RG32F								0x8230
#define GL_RGBA32F								0x8814
#define GL_RGB32F								0x8815
#define GL_RGBA16F								0x881A
#define GL_RGB16F								0x881B
#define GL_RGBA32UI								0x8D70
#define GL_RGB32UI								0x8D71
#define GL_RGBA16UI								0x8D76
#define GL_RGB16UI								0x8D77
#define GL_RGBA8UI								0x8D7C
#define GL_RGB8UI								0x8D7D
#define GL_RGBA32I								0x8D82
#define GL_RGB32I								0x8D83
#define GL_RGBA16I								0x8D88
#define GL_RGB16I								0x8D89
#define GL_RGBA8I								0x8D8E
#define GL_RGB8I								0x8D8F
#define GL_TEXTURE0								0x84C0
#define GL_TEXTURE1								0x84C1
#define GL_TEXTURE2								0x84C2
#define GL_TEXTURE3								0x84C3
#define GL_TEXTURE4								0x84C4
#define GL_CLIENT_ACTIVE_TEXTURE				0x84E1
#define GL_FRAGMENT_SHADER						0x8B30
#define GL_VERTEX_SHADER						0x8B31
#define GL_COMPILE_STATUS						0x8B81
#define GL_INFO_LOG_LENGTH						0x8B84
#define GL_CURRENT_PROGRAM						0x8B8D
#define GL_FRAMEBUFFER							0x8D40
#define GL_BGR									0x80E0
#define GL_ACTIVE_TEXTURE						0x84E0
#define GL_FRAMEBUFFER_COMPLETE					0x8CD5
#define GL_RENDERBUFFER							0x8D41
#define GL_DEPTH24_STENCIL8						0x88F0
#define GL_COLOR_ATTACHMENT0					0x8CE0
#define GL_DEPTH_ATTACHMENT						0x8D00
#define GL_RENDERBUFFER_WIDTH					0x8D42
#define GL_RENDERBUFFER_HEIGHT					0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT			0x8D44
#define GL_RENDERBUFFER_RED_SIZE				0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE				0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE				0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE				0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE				0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE			0x8D55
#define GL_LINK_STATUS							0x8B82

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_VERSION_ARB               0x2095
#define ERROR_INVALID_PROFILE_ARB               0x2096
#define GL_DEBUG_OUTPUT							0x92E0
#define GL_STACK_OVERFLOW						0x0503
#define GL_STACK_UNDERFLOW						0x0504
#define GL_DEBUG_OUTPUT_SYNCHRONOUS				0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH		0x8243
#define GL_DEBUG_CALLBACK_FUNCTION				0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM			0x8245
#define GL_DEBUG_SOURCE_API						0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM			0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER			0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY				0x8249
#define GL_DEBUG_SOURCE_APPLICATION				0x824A
#define GL_DEBUG_SOURCE_OTHER					0x824B
#define GL_DEBUG_TYPE_ERROR						0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR		0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR		0x824E
#define GL_DEBUG_TYPE_PORTABILITY				0x824F
#define GL_DEBUG_TYPE_PERFORMANCE				0x8250
#define GL_DEBUG_TYPE_OTHER						0x8251
#define GL_MAX_DEBUG_MESSAGE_LENGTH				0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES			0x9144
#define GL_DEBUG_LOGGED_MESSAGES				0x9145
#define GL_DEBUG_SEVERITY_HIGH					0x9146
#define GL_DEBUG_SEVERITY_MEDIUM				0x9147
#define GL_DEBUG_SEVERITY_LOW					0x9148
#define GL_DEBUG_TYPE_MARKER					0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP				0x8269
#define GL_DEBUG_TYPE_POP_GROUP					0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION			0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH			0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH				0x826D
#define GL_BUFFER								0x82E0
#define GL_SHADER								0x82E1
#define GL_PROGRAM								0x82E2
#define GL_QUERY								0x82E3
#define GL_PROGRAM_PIPELINE						0x82E4
#define GL_SAMPLER								0x82E6
#define GL_DISPLAY_LIST							0x82E7
#define GL_MAX_LABEL_LENGTH						0x82E8
#define GL_DEBUG_OUTPUT							0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT				0x00000002
#define WGL_NUMBER_PIXEL_FORMATS_ARB            0x2000
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_DRAW_TO_BITMAP_ARB                  0x2002
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_NEED_PALETTE_ARB                    0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB             0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB              0x2006
#define WGL_SWAP_METHOD_ARB                     0x2007
#define WGL_NUMBER_OVERLAYS_ARB                 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                0x2009
#define WGL_TRANSPARENT_ARB                     0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB           0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB         0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB          0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB         0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB         0x203B
#define WGL_SHARE_DEPTH_ARB                     0x200C
#define WGL_SHARE_STENCIL_ARB                   0x200D
#define WGL_SHARE_ACCUM_ARB                     0x200E
#define WGL_SUPPORT_GDI_ARB                     0x200F
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_STEREO_ARB                          0x2012
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_RED_SHIFT_ARB                       0x2016
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_GREEN_SHIFT_ARB                     0x2018
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_BLUE_SHIFT_ARB                      0x201A
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_ALPHA_SHIFT_ARB                     0x201C
#define WGL_ACCUM_BITS_ARB                      0x201D
#define WGL_ACCUM_RED_BITS_ARB                  0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                0x2021
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_AUX_BUFFERS_ARB                     0x2024
#define WGL_NO_ACCELERATION_ARB                 0x2025
#define WGL_GENERIC_ACCELERATION_ARB            0x2026
#define WGL_FULL_ACCELERATION_ARB               0x2027
#define WGL_SWAP_EXCHANGE_ARB                   0x2028
#define WGL_SWAP_COPY_ARB                       0x2029
#define WGL_SWAP_UNDEFINED_ARB                  0x202A
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_TYPE_COLORINDEX_ARB                 0x202C
#define WGL_ARB_multisample 1
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

#endif

#if defined( GL_EXT_LOAD_AMD_EXT )

#define GL_DOPP_GRID_ROW                0x931A
#define GL_DOPP_GRID_COLUMN             0x931B
#define GL_WAIT_FOR_PREVIOUS_VSYNC		0x931C

#endif //defined( GL_EXT_LOAD_AMD_EXT )

#endif // !defined( GLEXT_DEFINES_DEFINED )

#ifdef WIN32
	GL_EXT_EXP( void,	glActiveTexture, ( GLenum texture ) )		//!< Pointer to glActiveTexture function. 
	GL_EXT_EXP( void,	glClientActiveTexture, ( GLenum texture ) )		//!< Pointer to glActiveTexture function. 
	GL_EXT_EXP( void,	glUniform1i, ( GLint location, GLint v1 ) )
	GL_EXT_EXP( void,	glUniform4f, ( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ) )
	GL_EXT_EXP( void,	glUniform1f, ( GLint location, GLfloat v ) )
	GL_EXT_EXP( void,	glUniform1fv, ( GLint location, GLsizei count, const GLfloat* v ) )
	GL_EXT_EXP( void,	glUniform4fv, ( GLint location, GLsizei count, const GLfloat* v0 ) )
	GL_EXT_EXP( void,	glUniformMatrix2fv, ( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) )
	GL_EXT_EXP( void,	glUniformMatrix3fv, ( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) )
	GL_EXT_EXP( void,	glUniformMatrix4fv, ( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value ) )
	GL_EXT_EXP( GLuint,	glCreateProgram, () )
	GL_EXT_EXP( GLuint,	glCreateShader, ( GLenum type ) )
	GL_EXT_EXP( void,	glShaderSource, ( GLuint shader, GLsizei count, const GLchar* *string, const GLint *length ) )
	GL_EXT_EXP( void,	glCompileShader, ( GLuint shader ) )
	GL_EXT_EXP( void,	glValidateProgram, ( GLuint program ) )
	GL_EXT_EXP( void,	glDeleteProgram, ( GLuint program ) )
	GL_EXT_EXP( void,	glDeleteShader, ( GLuint shader ) )
	GL_EXT_EXP( void,	glAttachShader, ( GLuint program, GLuint shader ) )
	GL_EXT_EXP( void,	glDetachShader, ( GLuint program, GLuint shader ) )
	GL_EXT_EXP( void,	glLinkProgram, ( GLuint program ) )
	GL_EXT_EXP( GLint,	glGetUniformLocation, ( GLuint program, const GLchar *name ) )
	GL_EXT_EXP( void,	glGetShaderiv, ( GLuint shader, GLenum pname, GLint *params ) )
	GL_EXT_EXP( void,	glGetShaderInfoLog, ( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar* infoLog ) )
	GL_EXT_EXP( void,	glGetProgramInfoLog, ( GLuint program, GLsizei bufSize, GLsizei *length, GLchar* infoLog ) )
	GL_EXT_EXP( void,	glUseProgram, ( GLuint prog ) )

	GL_EXT_EXP( void,	glGenFramebuffers, ( GLuint n, GLuint* buffers ) )
	GL_EXT_EXP( void,	glBindFramebuffer, ( GLenum target, GLuint framebuffer ) )
	GL_EXT_EXP( void,	glDeleteFramebuffers, ( GLuint n, GLuint* buffers ) )
	GL_EXT_EXP( void,	glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) )
	GL_EXT_EXP( void,	glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) )
	GL_EXT_EXP( GLenum,	glCheckFramebufferStatus, (GLenum target) )

	GL_EXT_EXP( void,	glGenRenderbuffers, ( GLuint n, GLuint* buffers ) )
	GL_EXT_EXP( void,	glBindRenderbuffer, (GLenum target, GLuint renderbuffer) )
	GL_EXT_EXP( void,	glDeleteRenderbuffers, ( GLuint n, GLuint* buffers ) )

	GL_EXT_EXP( void,	glGenBuffers, (GLsizei n, GLuint *buffers))
	GL_EXT_EXP( void,	glBindBuffer, (GLenum target, GLuint buffer))
	GL_EXT_EXP( void,	glBufferData, (GLenum target, GLsizeiptrARB size, const void *data, GLenum usage))
	GL_EXT_EXP( void,	glDeleteBuffers, (GLsizei n, const GLuint *buffers))
	GL_EXT_EXP( void,	glGenVertexArrays, (GLsizei n, GLuint *arrays))
	GL_EXT_EXP( void,	glBindVertexArray, (GLuint iarray))
	GL_EXT_EXP( void,	glDeleteVertexArrays, (GLsizei n, const GLuint *arrays))

	GL_EXT_EXP( void,	glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer))
	GL_EXT_EXP( void,	glEnableVertexAttribArray, (GLuint index))
	GL_EXT_EXP( void,	glDisableVertexAttribArray, (GLuint index))
	GL_EXT_EXP( GLint,  glGetAttribLocation, (GLuint program, const GLchar* name))
	GL_EXT_EXP( void,	glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) )
	GL_EXT_EXP( void,	glGetRenderbufferParameteriv, ( 	GLuint renderbuffer,  	GLenum pname,  	GLint *params) )
	GL_EXT_EXP( void,	glGetBufferParameteriv, (GLenum target,  GLenum value,  GLint * data) )
	GL_EXT_EXP( void,	glGetProgramiv, (GLuint program, GLenum pname, GLint* params ) )
	GL_EXT_EXP( HGLRC,  wglCreateContextAttribsARB, (HDC hDC, HGLRC hshareContext, const int *attribList) )
	GL_EXT_EXP( BOOL,   wglGetPixelFormatAttribivARB, (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues))
	GL_EXT_EXP( BOOL,   wglChoosePixelFormatARB, (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats))
	GL_EXT_EXP( void,   glDebugMessageCallback, (DEBUGPROC callback, void* userParam) )
#endif

#if defined( GL_EXT_LOAD_AMD_EXT )
	GL_EXT_EXP( GLuint,	wglGetDesktopTextureAMD, () )
	GL_EXT_EXP( void,	wglEnablePostProcessAMD, (bool enable) )
	GL_EXT_EXP( GLuint,	wglPresentTextureToVideoAMD, (GLuint presentTexture, const GLuint* attrib_list) )
	GL_EXT_EXP( GLuint,	wglGenPresentTextureAMD, () )
	GL_EXT_EXP( GLint,	wglGetDisplayOverlapsAMD, (GLuint, GLuint) )
	GL_EXT_EXP( bool,	wglDesktopTargetAMD, (GLuint desktopIndex) )
#endif //defined( GL_EXT_LOAD_AMD_EXT )

#undef GL_EXT_EXP
