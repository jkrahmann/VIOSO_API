// ############################### //
// ## VIOSO Warp and Blend dll  ## //
// ## for DirectX + OpenGL      ## //
// ## author:  Juergen Krahmann ## //
// ## version: 1.3.0            ## //
// ## date:    2019/03/07       ## //
// ## Use with Calibrator >=4.3 ## //
// ## Copyright VIOSO GmbH 2015 ## //
// ############################### //

#include "VWBTypes.h"
#include "VWBDef.h"

/**
@file VIOSO API main include header
@brief
This library is meant to use in image generators, to do warping and blending. It takes a .vwf export and a texture buffer 
to sample from. If no texture buffer is given, it uses a copy of the current back buffer. It will render to the currently set back buffer.
It provides image based warping, suitable for most cases and, if a 3D map is provided, dynamc eye warping.
Usage: 
1. Static binding, having VIOSOWarpBlend next to your executable or in added to %path%:
link against VIOSOWarpBlend.lib and, in your [precompiled] header.
#include "VIOSOWarpBlend.h"

2. Dynamic binding, giving a path to load dynamic library from:
#define VIOSOWARPBLEND_FILE with a path (relative to main executable), this defaults to VIOSOWarpBlend / VIOSOWarpBlend64
2a) via [precompiled] header:
in header declare functions and types:
#define VIOSOWARPBLEND_DYNAMIC_DEFINE
#include "VIOSOWarpBlend.h"

in one file on top, to implement the actual functions/objects
#define VIOSOWARPBLEND_DYNAMIC_IMPLEMENT
#include "VIOSOWarpBlend.h"

in module initialization, this loads function pointers from library
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"

---
2b) Single file
in file on top, to declare and implement functions/objects,
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "VIOSOWarpBlend.h"

in module initialization, this loads function pointers from library
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"

Always make sure to have your platform headers loaded before!
To access deprecated functions #define VWB_USE_DEPRECATED_INIT

@Remarks
To allocate a warper, call VWB_Create. A specified config file is read.
You might edit all settings in warper struct. Then call VWB_Init.
There is no logical maximum on the number of allocated warpers.
Call VWB_getViewProj to obtain view and projection matrices relative to some eye point.
Call VWB_render to warp scene to screen. There is no multi-threading inside VIOSOWarpBlend module, make sure to use same thread or make your GL-context current before calling VWB_render.

You need VIOSO Calibrator to export a warp map as Vioso Warp File.
This API can use all mappings. In case you export 3D, you have to specify view parameter.
The eye point correction algorithm works implicitly. Imagine a rectangular "window" placed virtually near next to the screen.
It must be situated the way that from every possible (dynamic) eye point the whole projection area of the regarded projector
is seen through tat window.
This leads to an asymetrical frustum for a dynamic eye-point, which leads us to a projection-view-matrix mPV.
If you use that matrix in conjunction with the world matrix, one can render this "window" to provide all information needed
on the screen for that projector.
The map provided for the projector has 3D-coordinates of the actual screen for every pixel. If you multiply such a 
3D coordinate with mPV it leads to the u-v-coordinate in the "window". So if we sample in the provided backbuffer,
we can fill every pixel with content.
Here we define the rectangle. We do that in the familiar way of defining a standard frustum. The only additional information s the 
distance, where the screen will be (screen). These values come from a configuraton file or can be entered in settings.
Example .ini file (comments are actually not allowed there, remove them before use!!):
; this is where values for every channel go...
[default]
logLevel=1
logFile=VIOSOWarpBlend.log			;some other log file; it defaults to VIOSOWarpBlend.log, relative paths are relative to VIOSOWarpBlend.dll, not the main module!
bLogClear=0							; if 1, clear the logfile on start, defaults to 0
near=1.0							; the near plane 
far=20000.0							; the far plane
trans=[1,0,0,0;0,1,0,0;0,0,1,0;0,0,0,1] ;this is the transformation matrix transforming a vioso coordinate to an IG coordinate this defines the pivot of a moving platform, column major format
base=[1,0,0,0;0,1,0,0;0,0,-1,0;0,0,0,1] ;same as above, row major format DirectX conform, as DX uses left-handed coordinate system, z has to be inverted! 
bTurnWithView=1						;set to 1 if view turnes and moves with eye, i.e view is obtained from a vehicle position on a moving platform, defaults to 0
bBicubic=0							;set to 1 to enable bicubic content texture filter, defaults to 0
bDoNotBlend=0						;set to 1 to disable blending, defaults to 0
splice=0							;bitfield:	0x00000001 change sign of pitch, 0x00000002 use input yaw as pitch, 0x00000004 use input roll as pitch
									;0x00000010 change sign of yaw, 0x00000020 use input pitch as yaw, 0x00000040 use input roll as yaw
									;0x00000100 change sign of roll, 0x00000200 use input pitch as roll, 0x00000400 use input yaw as roll
									;0x00010000 change sign of x movement, 0x00020000 use input y as x, 0x00040000 use input z as x
									;0x00100000 change sign of y movement, 0x00200000 use input x as y, 0x00400000 use input z as y
									;0x01000000 change sign of z movement, 0x02000000 use input x as z, 0x04000000 use input y as z
bAutoView=0							;set to 1 to enable automatic view calculation, defaults to 0
autoViewC=1							;moving range coefficient. defaults to 1, set a range in x,y,z from platform origin to be mapped; x+- autoViewC * screen / 2
gamma=1.0							;set a gamma value. This is multiplied by the gamma already set while calibrating!, defaults to 1
port=942							;set to a TCP IPv4 port, use 0 to disable network. defaults to 0, standard port is 942
addr=0.0.0.0						;set to IPv4 address, to listen to specific adapter, set to 0.0.0.0 to listen on every adapter, defaults to 0.0.0.0
bUseGL110=0							;set to 1, to use shader version 1.1 with fixed pipeline, defaults to 0
bPartialInput						;set to 1, to input texture act as optimal rect part, defaults to 0

[channel 1]
eye=[0,0,0]							; this is the eye position relative to pivot
dir=[0, -20, 0]						; this is the view direction, rotation angles around axis' [x,y,z]. The rotation order is yaw (y), pitch (x), roll (z); positive yaw turns right, positive pitch turns up and positive roll turns clockwise
fov=[45,45,45,45]					; this is the fields of view, x - left, y - top, z - right, w - bottom
screen=3650.000000					; the distance of the render plane from eye point
calibFile=..\Res\Calib_150930_3.vwf ; path to warp blend file(s), if relative, it is relative to the VIOSOWarpBlend.dll, not the main module! To load more than .vwf/.bmp, separate with comma.
calibIndex=0						; index, in case there are more than one display calibrated; if index is omitted you can specify the display ordinal
calibAdapterOrdinal=1				; display ordinal the number in i.e. "D4 UHDPROJ (VVM 398)" matches 4. In case no index is found index is set to 0
eyePointProvider=EyePointProvider	; a eye point provider dll name; the name is passed to LoadLibrary, so a full qualified path is possible, use quotes if white spaces
eyePointProviderParam=				; string used to initialize provider, this depends on the implmementation
mouseMode=0							; bitfield, set 1 to render current mouse cursor on top of warped buffer

;[channel 2] next channel, if you are using this .ini for more than one channel
*/

	/** creates a new VIOSO Warp & Blend API instance
	* @param [IN_OPT] pDxDevice  a pointer to a DirectX device, set to NULL for OpenGL, set to VWB_DUMMYDEVICE, to just hold the data to create a textured mesh.
	* Supported: IDirect3DDevice9,IDirect3D10Device,IDirect3D11Device
	* @param [IN_OPT] szConfigFile  path to a .ini file containing settings, if NULL the default values are used
	* @param [IN_OPT] szChannelName a section name to look for in .ini-file.
	* @param [OUT] ppWarper this receives the warper
	* @param [IN_OPT] logLevel, set a log level.
	* @param [IN_OPT] logFile, set a log file.
	* @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	* @remarks Default settings are loaded, then, if a .ini file is given, the settings are updated.
	* If there is a [default] section, values are loaded from there, before individual settings are loaded.
	* After creating, you are able to edit all settings. You may specify a logLevel in .ini, it overwrites given loglevel.
	* You have to destroy a created warper using VWB_Destroy.
	* After successful creation, call VWB_Init, to load .vwf file and initialize the warper. */
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_CreateA, ( void* pDxDevice, char const* szConfigFile, char const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char const* szLogFile ) );   
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_CreateW, ( void* pDxDevice, wchar_t const* szConfigFile, wchar_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, wchar_t const* szLogFile ) );   
#ifdef UNICODE
#define VWB_Create VWB_CreateW
#else //def UNICODE
#define VWB_Create VWB_CreateA
#endif //def UNICODE

	/** destroys a warper
	* @param pWarper	a warper to be destroyed */
	VIOSOWARPBLEND_API( void, VWB_Destroy, ( VWB_Warper* pWarper ));

	/** initializes a warper
	* @param pWarper	a warper 
	* @remarks In case of an OpenGL warper, the later used context needs to be active. There is no multi-threading on side of VIOSO API. */
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_Init, ( VWB_Warper* pWarper ));

    /** set the new dynamic eye position and direction
	* @param [IN]			pWarper	a valid warper
    * @param [IN,OUT,OPT]	pEye	sets the new eye position, if a eye point provider is present, the value is getted from there, pEye is updated
    * @param [IN,OUT,OPT]	pRot	the new rotation in radian, if a eye point provider is present, the value is getted from there, pRot is updated
    * @param [OUT]			pView	it gets the updated view matrix to translate and rotate into the viewer's perspective
	* @param [OUT]			pProj	it gets the updated projection matrix
	* @param [OUT]			pClip	it gets the updated clip planes, left, top, right, bottom, near, far
    * @return VWB_ERROR_NONE on success, VWB_ERROR_GENERIC otherwise
	* @remarks If EyePointProvider is used, the eye point is set by calling it's getEye function. eye and rot are set to that if not NULL.
	* Else, if eye and rot are not NULL, values taken from here.
	* Else the eye and rot are set to 0-vectors.
	* The internal view and projection matrices are calculated to render. You should set pView and pProj to get these matrices for rendering, if updated.
	* positive rotation means turning right, up and clockwise.
	* 	use these functions to get euler angles
	// gets euler angles from a GL-style 3x3 rotation matrix in (right-handed coordinate system, z backward, y up and x right)
	// assumptions:
	// positive rotation around x turns (pitch) up
	// positive rotation around y turns (yaw) right
	// positive rotation around z turns (roll) clockwise
	// rotation order is y-x-z, this corresponds to R()
	_inline_ VWB_VECTOR3<_T> GetR_RH() const
	{
		VWB_VECTOR3<_T> a; 
		a.y = atan2( p[6], p[8] );
		_T s = -sin( a.y );
		_T c = cos( a.y );
		_T l = sqrt( p[6] * p[6] + p[8] * p[8] );
		a.x = atan2( p[7], l );
		a.z = -atan2( s * p[5] + c * p[3], s * p[2] + c * p[0] );
		return a;
	}

	// gets euler angles from a DX-style 3x3 rotation matrix in (transposed left-handed coordinate system, z forward, y up and x right)
	// assumptions:
	// positive rotation around x turns (pitch) up
	// positive rotation around y turns (yaw) right
	// positive rotation around z turns (roll) clockwise
	// rotation order is y-x-z, this corresponds to RT()
	_inline_ VWB_VECTOR3<_T> GetR_LHT() const
	{
		VWB_VECTOR3<_T> a;
		a.y = atan2( -p[2], p[8] );
		_T s = sin( a.y );
		_T c = cos( a.y );
		_T l = sqrt( p[2] * p[2] + p[8] * p[8] );
		a.x = -atan2( p[5], l );
		a.z = -atan2( s * p[7] + c * p[1], s * p[6] + c * p[0] );
		return a;
	}
*/
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_getViewProj, ( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj ) );
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_getViewClip, ( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip ) );

    /** set the view and projection matrix directly
	* @param [IN]			pWarper	a valid warper
    * @param [IN]			pView	view matrix to translate and rotate into the viewer's perspective
    * @param [IN]			pProj	projection matrix
    * @return VWB_ERROR_NONE on success, VWB_ERROR_GENERIC otherwise */
	VIOSOWARPBLEND_API( VWB_ERROR, VWB_setViewProj, ( VWB_Warper* pWarper, VWB_float* pView,  VWB_float* pProj) );

    /** render a warped and blended source texture into the current back buffer
	* @remark for DirectX 9, call inside BeginScene/EndScene statements
	* @param [IN]			pWarper	a valid warper
    * @param [IN,OPT]		pSrc    the source texture, a IDirect3DTexture9* or a GLint texture index; if current backbuffer must be read, set to NULL in any DX mode or to -1 in OpenGL mode
	* @param [IN,OPT]		stateMask @see VWB_STATEMASK enumeration, default is 0 to restore usual stuff
    * @return VWB_ERROR_NONE on success, VWB_ERROR_GENERIC otherwise */
    VIOSOWARPBLEND_API( VWB_ERROR, VWB_render, ( VWB_Warper* pWarper, VWB_param src, VWB_uint stateMask ) );  

	/** get info about .vwf, reads all warp headers
	* @param [IN]			path	the file name or a comma separated list of filenames, set to NULL to release data from a previous set
	* @param [OUT]			set		a warp blend header set
	* @return VWB_ERROR_NONE on success, VWB_ERROR_PARAM if fname is not set or empty, VWB_ERROR_VWF_FILE_NOT_FOUND if path did not resolve, VWB_ERROR_GENERIC otherwise
	* @remarks The list is empied and all found headers are appended. */
    VIOSOWARPBLEND_API( VWB_ERROR, VWB_vwfInfo, ( char const* path, VWB_WarpBlendHeaderSet* set ) );  

    /** fills a VWB_WarpBlendMesh from currently loaded data. It uses cols * rows vertices or less depending on how .vwf is filled. Warper needs to be initialized as VWB_DUMMYDEVICE.
	* @param [IN]			pWarper	a valid warper
	* @param [IN]			cols	sets the number of columns
	* @param [IN]			rows	sets the number of rows
	* @param [OUT]			mesh	the resulting mesh, the mesh will be emptied before filled
    * @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER, if parameters are out of range, VWB_ERROR_GENERIC otherwise */
    VIOSOWARPBLEND_API( VWB_ERROR, VWB_getWarpBlendMesh, ( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) );

	/* log some string to the API's log file, exposed version; use VWB_logString instead.
	* @param [IN]			level	a level indicator. The string is only written to log file, if this is lower or equal to currently set global log level
	* @param [IN]			str		a null terminated multibyte character string
    * @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER otherwise */
    VIOSOWARPBLEND_API( VWB_ERROR, VWB__logString, ( VWB_int level, char const* str ) );



#if defined( WIN32 ) && defined( VWB_USE_DEPRECATED_INIT )
//////////////////// deprecated WIN32 functions ////////////////////////
typedef VWB_Warper* VWB_DX9WHANDLE; /// (deprecated) a warper handle do
// forward declarations only, your project needs to include d3d9.h and d3d9x.h
typedef struct IDirect3DDevice9 *LPDIRECT3DDEVICE9;
typedef struct IDirect3DTexture9 *LPDIRECT3DTEXTURE9;
struct D3DXVECTOR4;
struct D3DXMATRIX;

	/** (deprecated)* if Init returns 0, get error
	* @return	 0 .. no error
	*			-1 .. generic error
	*			-2 .. parameter error
	*			-4 .. blend texture failed
	*			-5 .. warp texture failed
	*			-6 .. shader error
	*			-7 .. vwf load failure 
	*			-8 .. vwf file not found */
	VIOSOWARPBLEND_API( DWORD, VWB_GetError, () );   

	/** InitDX9 InitDX9A InitDX9W (deprecated) for compatibility reasons
	* create and initialize a warper
    * @param pDevice [IN]			a pointer to a IDirect3DDevice9
	* @param szConfigFile [IN,OPT]	a zero terminated string pointing to a config file, if set to NULL, the plugin will look for a VIOSOWarpBlend.ini next to the .dll
	* @param szChannel [IN,OPT]	a zero terminated string pointing to a section name in the .ini file, if set to NULL, the plugin will look for "channel1"
	* @return a nonzero DX9WHANDLE on success, 0 otherwise. Use GetError() for more information. */
	VIOSOWARPBLEND_API( VWB_DX9WHANDLE, VWB_InitDX9A, ( LPDIRECT3DDEVICE9 pDevice, LPCSTR szConfigFile, LPCSTR szChannel ));
	VIOSOWARPBLEND_API( VWB_DX9WHANDLE, VWB_InitDX9W, ( LPDIRECT3DDEVICE9 pDevice, LPCWSTR szConfigFile, LPCWSTR szChannel ));
#ifdef UNICODE
#define VWB_InitDX9 VWB_InitDX9W
#else
#define VWB_InitDX9 VWB_InitDX9A
#endif
	/** Destroy
	* Destroy a directx 9 warper (deprecated) for compatibility reasons
	* @param h		a warper handle to be destroyed */
	VIOSOWARPBLEND_API( void, VWB_DestroyDX9, ( VWB_DX9WHANDLE h ));

    /** updateViewProjDX9 (deprecated) for compatibility reasons
	* set the new dynamic eye position for a DX9 environment
	* @param [IN]		h		a valid warper handle
    * @param [IN,OUT,OPT] pEye	the new eye position, if a eye point provider is present, the value is getted from there, pEye is updated
    * @param [IN,OUT,OPT] pRot	the new rotation in radian, if a eye point provider is present, the value is getted from there, pRot is updated
    * @param [OUT]		pView	it gets the updated view matrix to translate and rotate into the viewer's perspective
    * @param [OUT]		pProj	it gets the updated projection matrix
    * @return TRUE on success, FALSE otherwise */
	VIOSOWARPBLEND_API( BOOL, VWB_updateViewProjDX9, ( VWB_DX9WHANDLE h, D3DXVECTOR4 * pEye, D3DXMATRIX* pView, D3DXMATRIX* pProj));
	VIOSOWARPBLEND_API( BOOL, VWB_updateViewProj2DX9, ( VWB_DX9WHANDLE h, D3DXVECTOR4 * pEye, D3DXVECTOR4 * pRot, D3DXMATRIX* pView, D3DXMATRIX* pProj));

    /** renderDX9 (deprecated) for compatibility reasons
	* render a warped and blended source texture into the current back buffer for DX9 environment
	* @remark you have to call inside BeginScene/EndScene statement
	* @param h		a valid warper handle
    * @param pSrc    the source texture
    * @return S_OK on success, E_FAIL otherwise */
    VIOSOWARPBLEND_API( HRESULT, VWB_renderDX9, ( VWB_DX9WHANDLE h, LPDIRECT3DTEXTURE9 pSrc ) );  
#endif //defined( WIN32 ) && defined( VWB_USE_DEPRECATED_INIT )


#undef VIOSOWARPBLEND_API