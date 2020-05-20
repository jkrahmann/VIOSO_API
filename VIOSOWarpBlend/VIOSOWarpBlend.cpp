#ifdef WIN32
#include "DX/DX9WarpBlend.h"
#include "DX/DX9EXWarpBlend.h"
#include "DX/DX11WarpBlend.h"
#include "DX/DX12WarpBlend.h"
#include "DX/DX10WarpBlend.h"
#include "Net.h"
#pragma comment( lib, "shlwapi" )
#include <shlwapi.h>
#else
#include <dlfcn.h>
#endif
#include "GL/GLWarpBlend.h"
#include <locale.h>
#include <time.h>
#include <limits.h>
#include <float.h>

#ifdef WIN32
#include <crtdbg.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

#include "logging.h"
#include "3rdparty/delauney/DelaunayTriangles.h"

VWB_size _size0 = { 0,0 };
#ifdef WIN32
Server g_server;
typedef std::vector<SPtr<VWBTCPListener>> ListenerList;
ListenerList g_listeners;
#endif

#ifdef WIN32
HMODULE g_hModDll = 0;
VWB_int g_error = 0;
HCURSOR g_hCur = 0;
SIZE	g_dimCur = { 0 };
POINT   g_hotCur = { 0 };
bool g_bCurEnabled = true;
FPtrInt_BOOL ShowSystemCursor = NULL;
#endif

#ifdef WIN32
DWORD VWB_GetError()
{
	return g_error;
}
#endif
VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out );

VWB_ERROR VWB_Warper_base::ReadIniFile( char const* szConfigFile, char const* szChannelName )
{
	if( szConfigFile && szConfigFile[0] )
	{
		Defaults();
		strcpy( path, szConfigFile );
		MkPath( path, MAX_PATH, ".ini" );

		if( szChannelName )
			strcpy( channel, szChannelName );
		else
			strcpy( channel, "default" );

		FILE* f = NULL;
		if( NO_ERROR == fopen_s( &f, path, "r" ) )
			fclose(f);
		else
			return VWB_ERROR_INI_LOAD;

		char s[1024];
		char sDef[MAX_PATH + 1024];
		int iDef;

		GetIniString( "default", "logFile", g_logFilePath, sDef, MAX_PATH, path );
		GetIniString( channel, "logFile", sDef, g_logFilePath, MAX_PATH, path );

		iDef = GetIniInt( "default", "logLevel", g_logLevel, path );
		g_logLevel = GetIniInt( channel, "logLevel", iDef, path );

		iDef = GetIniInt( "default", "bLogClear", 0, path );
		if( 1 == GetIniInt( channel, "bLogClear", iDef, path ) )
			logClear();
		else
			logStr( 1, "-VIOSO WARP BLEND API-\n" );

		iDef = GetIniInt( "default", "bTurnWithView", 0, path );
		bTurnWithView = 0 != GetIniInt( channel, "bTurnWithView", iDef, path );

		iDef = GetIniInt( "default", "bDoNotBlend", 0, path );
		bDoNotBlend = 0 != GetIniInt( channel, "bDoNotBlend", iDef, path );

		GetIniString( "default", "eyePointProvider", NULL, sDef, MAX_PATH, path );
		GetIniString( channel, "eyePointProvider", sDef, eyeProvider, MAX_PATH, path );
#if defined( WIN32 )
		char const* ext = ".dll";
        MkPath( eyeProvider, MAX_PATH, ext );
#elif defined( __APPLE__ )
        // no eye provider
#else
		char const* ext = ".so";
        MkPath( eyeProvider, MAX_PATH, ext );
#endif // defined( WIN32 )

		GetIniString( "default", "eyePointProviderParam", NULL, sDef, MAX_PATH, path );
		GetIniString( channel, "eyePointProviderParam", sDef, eyeProviderParam, MAX_PATH, path );

		GetIniString( "default", "calibFile", "vioso.vwf", sDef, MAX_PATH, path );
		GetIniString( channel, "calibFile", sDef, calibFile, 12 * MAX_PATH, path );

		iDef = GetIniInt( "default", "calibIndex", -1, path );
		calibIndex = GetIniInt( channel, "calibIndex", iDef, path );
		if( -1 == calibIndex )
		{
			iDef = GetIniInt( channel, "calibAdapterOrdinal", 0, path );
			calibIndex = -1 * ( GetIniInt( channel, "calibAdapterOrdinal", iDef, path ) );
		}
	
		VWB_float const _Zero[16] = { 0 };
		GetIniMat("default", "eye", 3, 1, _Zero, eye, path);

		GetIniString( "default", "dir", "[0,0,0]", sDef, 1024, path );
		GetIniString( channel, "dir", sDef, s, 1024, path );
		sscanf_s( s, "[%f,%f,%f]", &dir[0], &dir[1], &dir[2] );

		GetIniString( "default", "fov", "[35,30,35,30]", sDef, 1024, path );
		GetIniString( channel, "fov", sDef, s, 1024, path );
		sscanf_s( s, "[%f,%f,%f,%f]", &fov[0], &fov[1], &fov[2], &fov[3] );

		GetIniString( "default", "near", "0.1", sDef, 1024, path );
		GetIniString( channel, "near", sDef, s, 1024, path );
		sscanf_s( s, "%f", &nearDist );

		GetIniString( "default", "far", "10000", sDef, 1024, path );
		GetIniString( channel, "far", sDef, s, 1024, path );
		sscanf_s( s, "%f", &farDist );

		GetIniString( "default", "screen", "2000", sDef, 1024, path );
		GetIniString( channel, "screen", sDef, s, 1024, path );
		sscanf_s( s, "%f", &screenDist );

		GetIniString( "default", "trans", "", sDef, 1024, path );
		GetIniString( channel, "trans", sDef, s, 1024, path );
		if(	16 != sscanf_s( s, "[%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f]", 
						&trans[0], &trans[1], &trans[2], &trans[3],
						&trans[4], &trans[5], &trans[6], &trans[7],
						&trans[8], &trans[9], &trans[10], &trans[11],
						&trans[12], &trans[13], &trans[14], &trans[15] )
		  )
		{
			GetIniString( "default", "base", "[1,0,0,0;0,1,0,0;0,0,1,0;0,0,0,1]", sDef, 1024, path );
			GetIniString( channel, "base", sDef, s, 1024, path );
			sscanf_s( s, "[%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f;%f,%f,%f,%f]", 
						&trans[0], &trans[4], &trans[8], &trans[12],
						&trans[1], &trans[5], &trans[9], &trans[13],
						&trans[2], &trans[6], &trans[10], &trans[14],
						&trans[3], &trans[7], &trans[11], &trans[15] );
		}

		iDef = GetIniInt( "default", "mode", -1, path );
		iDef = GetIniInt( channel, "mode", iDef, path );
		if( -1 != iDef )
		{
			switch( iDef )
			{
			case 1: 
				splice = 0x00000122;
				break;
			case 2:
				splice = 0x00000133;
				break;
			default:
				splice = 0;
			}
		}
		else
		{
			iDef = GetIniInt( "default", "splice", 0, path );
			splice = GetIniInt( channel, "splice", iDef, path );
		}

		iDef = GetIniInt( "default", "bBicubic", -1, path );
		iDef = GetIniInt( channel, "bBicubic", iDef, path );
		if( -1 == iDef )
		{
			iDef = GetIniInt( "default", "bicubic", 0, path );
			bBicubic = 0 != GetIniInt( channel, "bicubic", iDef, path );
		}
		else
			bBicubic = 0 != iDef;

		iDef = GetIniInt( "default", "bUseGL110", 0, path );
		bUseGL110 = 0 != GetIniInt( channel, "bUseGL110", iDef, path );

		iDef = GetIniInt("default", "bPartialInput", 0, path);
		bPartialInput = 0 != GetIniInt(channel, "bPartialInput", iDef, path);

		iDef = GetIniInt("default", "mouseMode", 0, path);
		mouseMode = GetIniInt(channel, "mouseMode", iDef, path);

		GetIniString( "default", "autoViewC", "1.0", sDef, 1024, path );
		GetIniString( channel, "autoViewC", sDef, s, 1024, path );
		sscanf_s( s, "%f", &autoViewC );

		iDef = GetIniInt( "default", "bAutoView", 1, path );
		bAutoView = 0 != GetIniInt( channel, "bAutoView", iDef, path );

		GetIniString( "default", "optimalRes", "[0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRes", sDef, s, 1024, path );
		sscanf_s( s, "[%i,%i]", &optimalRes.cx, &optimalRes.cy );

		GetIniString( "default", "optimalRes", "[0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRes", sDef, s, 1024, path );
		sscanf_s( s, "[%d,%d]", &optimalRes.cx, &optimalRes.cy );

		GetIniString( "default", "optimalRect", "[0,0,0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRect", sDef, s, 1024, path );
		sscanf_s( s, "[%d,%d,%d,%d]", &optimalRect.left, &optimalRect.top, &optimalRect.right, &optimalRect.bottom );

		gamma = GetIniFloat( "default", "gamma", 1.0f, path );
		gamma = GetIniFloat( channel, "gamma", gamma, path );

		iDef = GetIniInt( "default", "port", 0, path );
		port = GetIniInt( channel, "port", iDef, path );

		GetIniString( "default", "addr", "0.0.0.0", sDef, MAX_PATH, path );
		GetIniString( channel, "addr", sDef, addr, MAX_PATH, path );

		iDef = GetIniInt( "default", "debugBreak", 0, path );
		if( GetIniInt( channel, "debugBreak", iDef, path ) )
		{
#ifdef WIN32
			MessageBox( NULL, "BREAK", "DEBUG", MB_OK );
#else
#endif
		}

	}
	return VWB_ERROR_FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////

VWB_ERROR VWB_CreateA( void* pDxDevice, char const* szConfigFile, char const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char const* szLogFile )
{
	if( NULL == ppWarper )
		return VWB_ERROR_PARAMETER;

	*ppWarper = NULL;
	g_logLevel = logLevel;
	if( szLogFile && szLogFile[0] )
		strcpy( g_logFilePath, szLogFile );
	else
		strcpy( g_logFilePath, "VIOSOWarpBlend" );
	MkPath( g_logFilePath, MAX_PATH, ".log" );

	try {
#ifdef WIN32
		if (VWB_DUMMYDEVICE == pDxDevice)
		{
			*ppWarper = new Dummywarper();
		}
		else if(pDxDevice)
		{
			IUnknown* pUK = (IUnknown*)pDxDevice;
			if( pUK )
			{
				IUnknown* pUK2 = NULL;
				// destinguish between DX flavours
				do {
					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D12CommandQueue ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX12WarpBlend( (ID3D12CommandQueue*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D11Device ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX11WarpBlend( (ID3D11Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device1 ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9Ex ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX9EXWarpBlend( (LPDIRECT3DDEVICE9EX)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9 ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX9WarpBlend( (LPDIRECT3DDEVICE9)pDxDevice );
							break;
						}
					}

				} while( 0 );
			}
		}
		else
#endif //def WIN32
		{
			*ppWarper = new GLWarpBlend();
		}
		if( !*ppWarper )
			throw (VWB_int)VWB_ERROR_GENERIC;
	} catch ( VWB_int e )
	{
		logStr( 0, "FATAL: Error %d creating warper \"%s\".\n", e, szChannelName ? szChannelName : (*ppWarper)->channel );
		return (VWB_ERROR)e;
	}
	if( NULL != szConfigFile && 0 != szConfigFile[0] )
	{
		if( !((VWB_Warper_base*)*ppWarper)->ReadIniFile( szConfigFile, szChannelName ) )
		{
			logStr( 0, "FATAL: .ini file (%s) parsing error.\n", szConfigFile );
			delete ((VWB_Warper_base*)*ppWarper);
			*ppWarper = NULL;
			return VWB_ERROR_INI_LOAD;
		}
	}
	else
	{
		if( NULL != szChannelName && 0 != szChannelName[0] )
			strcpy( ((VWB_Warper_base*)*ppWarper)->channel, szChannelName );
		else
			strcpy( ((VWB_Warper_base*)*ppWarper)->channel, "default" );
	}

	{
		time_t t;
		time( &t );
		struct tm tm = *localtime( &t );
		logStr( 1, "%04d/%02d/%02d VIOSOWarpBlend API %d.%d.%d.%d.\n", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, VWB_Version_MAJ, VWB_Version_MIN, VWB_Version_MAI, VWB_Version_REV );

		char* szModPath = NULL;
#ifdef WIN32
		char modPath[MAX_PATH] = {0};
		szModPath = modPath;
		if( ::GetModuleFileNameA( g_hModDll, modPath, MAX_PATH ) )
#else
		Dl_info dl_info;
		szModPath = (char*) dl_info.dli_fname;
		if( dladdr((void *)MkPath, &dl_info) )
#endif //def WIN32
		logStr( 1, "%s.\n", szModPath );
	}
	logStr( 2,	"%.4s-Warper \"%s\" created. Logging level is %d\nEvaluated parameters%s%s:\n"
				"calibFile=%s\n"
				"calibIndex=%d\n"
				"bTurnWithView=%d\n"
				"bDoNotBlend=%d\n"
				"eyePointProvider=%s\n"
				"eyePointProviderParam=%s\n"
				"eye=[%.5f, %.5f, %.5f]\n"
				"near=%.5f\n"
				"far=%.5f\n"
				"bBicubic=%d\n"
				"bUseGL110=%d\n"
				"bPartialInput=%d\n"
				"splice=%u\n"
				"trans=[%.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f]\n"
				"autoViewC=%.5f;\n"
				"bAutoView=%d;\n"
				"dir=[%.5f, %.5f, %.5f]\n"
				"fov=[%.5f, %.5f, %.5f, %.5f]\n"
				"screen=%.5f\n"
				"optimalRes=[%d, %d]\n"
				"optimalRect=[%d, %d, %d, %d]\n"
				"port=%d\n"
				"address=%s\n"
				"mouseMode=%d"
				"\n",
				((VWB_Warper_base*)*ppWarper)->GetType(), (*ppWarper)->channel, g_logLevel,
				(*ppWarper)->path[0] ? " from\n" : ", no .ini file set",
				(*ppWarper)->path,
				(*ppWarper)->calibFile,
				(*ppWarper)->calibIndex,
				(*ppWarper)->bTurnWithView ? 1 : 0,
				(*ppWarper)->bDoNotBlend ? 1 : 0,
				(*ppWarper)->eyeProvider,
				(*ppWarper)->eyeProviderParam,
				(*ppWarper)->eye[0],(*ppWarper)->eye[1],(*ppWarper)->eye[2],
				(*ppWarper)->nearDist,
				(*ppWarper)->farDist,
				(*ppWarper)->bBicubic ? 1 : 0,
				(*ppWarper)->bUseGL110 ? 1 : 0,
				(*ppWarper)->bPartialInput ? 1 : 0,
				(*ppWarper)->splice,
				(*ppWarper)->trans[ 0],(*ppWarper)->trans[ 1],(*ppWarper)->trans[ 2],(*ppWarper)->trans[ 3],
				(*ppWarper)->trans[ 4],(*ppWarper)->trans[ 5],(*ppWarper)->trans[ 6],(*ppWarper)->trans[ 7],
				(*ppWarper)->trans[ 8],(*ppWarper)->trans[ 9],(*ppWarper)->trans[10],(*ppWarper)->trans[11],
				(*ppWarper)->trans[12],(*ppWarper)->trans[13],(*ppWarper)->trans[14],(*ppWarper)->trans[15],
				(*ppWarper)->autoViewC,
				(*ppWarper)->bAutoView ? 1 : 0,
				(*ppWarper)->dir[0],(*ppWarper)->dir[1],(*ppWarper)->dir[2],
				(*ppWarper)->fov[0],(*ppWarper)->fov[1],(*ppWarper)->fov[2],(*ppWarper)->fov[3],
				(*ppWarper)->screenDist,
				(*ppWarper)->optimalRes.cx, (*ppWarper)->optimalRes.cy,
				(*ppWarper)->optimalRect.left, (*ppWarper)->optimalRect.top, (*ppWarper)->optimalRect.right, (*ppWarper)->optimalRect.bottom,  
				(*ppWarper)->port,
				(*ppWarper)->addr,
				(*ppWarper)->mouseMode
			);

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_CreateW( void* pDxDevice, wchar_t const* szConfigFile, wchar_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, wchar_t const* szLogFile )
{
	try
	{
		char szCF[MAX_PATH];
		char* pszCF = NULL;
		char szCN[MAX_PATH];
		char* pszCN = NULL;
		char szLF[MAX_PATH];
		char* pszLF = NULL;
		if( szConfigFile )
		{
			wcstombs( szCF, szConfigFile, MAX_PATH );
			pszCF = szCF;
		}
		if( szChannelName )
		{
			wcstombs( szCN, szChannelName, MAX_PATH );
			pszCN = szCN;
		}
		if( szLogFile )
		{
			wcstombs(szLF, szLogFile, MAX_PATH );
			pszLF = szLF;
		}
		return VWB_CreateA( pDxDevice, pszCF, pszCN, ppWarper, logLevel, pszLF );
	} catch(...)
	{
		return VWB_ERROR_PARAMETER;
	}
}

VWB_ERROR VWB_Init( VWB_Warper* pWarper )
{
	return VWB_InitExt( pWarper, nullptr );
}

VWB_ERROR VWB_InitExt( VWB_Warper* pWarper, VWB_WarpBlendSet* extSet )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	VWB_ERROR err = VWB_ERROR_NONE;

#ifdef _DEBUG
	if( pWarper->port )
	{
		SocketAddress sa = Socket::gethostbyname( "localhost" );
		if(0){
			TCPConnection conn( SocketAddress( "vioso.com", 80 ) );
			if( conn )
			{
				char sz[] = "GET / HTTP/1.1\015\012Host:www.vioso.com\015\012Client:WP\015\012\015\012";
				conn.sendDirect( sz, sizeof(sz)-1 );
				while( 0 == conn.getNumRead() )
					conn.processAsClient();
				char szRecv[1024] = "olla peter von frosta! Como e sta?";
				int bt = conn.readUntil( szRecv, 1024, "\015\012" );
				if( 0 == strcmp( szRecv, "HTTP/1.0 200 OK" ) 
				   || 0 == strcmp( szRecv, "HTTP/1.1 200 OK" ) )
				{
					bt = conn.readUntil( szRecv, 1024, "\015\012" );
				}
			}
		}
		std::vector<SPtr<VWBTCPListener>>::iterator it = g_listeners.begin();
		err = VWB_ERROR_FALSE;
		while( it != g_listeners.end() && VWB_ERROR_FALSE == ( err = (*it)->add( pWarper ) ) )	{ it++;	}
		if( VWB_ERROR_FALSE == err && it == g_listeners.end() )
		{
			g_listeners.push_back( new VWBTCPListener(SocketAddress( INADDR_ANY, (unsigned short)pWarper->port ) ) );
			it = g_listeners.end() - 1;
			err = (*it)->add( pWarper );
			if( VWB_ERROR_NONE != err )
			{
				logStr( 0, "ERROR(%i): failed to add warper \"%s\" to new listener at %hu.\n", err, pWarper->channel, pWarper->port );
				return err;
			}
			if( 0 != g_server.addReceiver( SPtr<SockIn>(*it) ) )
				err = VWB_ERROR_NETWORK;
		}
		if( VWB_ERROR_NONE != err )
		{
			logStr( 0, "ERROR(%i): failed to add warper \"%s\".\n", err, pWarper->channel );
			return err;
		}
		if( -1 == g_server.doModal() )
		{
			err = VWB_ERROR_NETWORK;
			return err;
		}
	}
#endif //def _DEBUG
	
	if( nullptr != extSet )
	{
		if ((VWB_ERROR_NONE != err) || !VerifySet(*extSet))
		{
			logStr(0, "ERROR: Failed to verify given set.\n");
			return err;
		}
		else
			err = VWB_ERROR_NONE;

		VWB_Warper_base* p = (VWB_Warper_base*)pWarper;
		err = p->Init( *extSet );
	}
	else
	{
		VWB_WarpBlendSet set;
		err = LoadVWF(set, ((VWB_Warper_base*)pWarper)->calibFile);
		//if( ( VWB_ERROR_VWF_LOAD == err ) ||
		//	( VWB_ERROR_VWF_FILE_NOT_FOUND == err && VWB_ERROR_NONE != CreateDummyVWF( set, ((VWB_Warper_base*)pWarper)->calibFile ) ) ||
		//    ( VWB_ERROR_NONE == err  && !VerifySet( set ) ) )

		if ((VWB_ERROR_NONE != err) || !VerifySet(set))
		{
			logStr(0, "ERROR: LoadVWF: Failed to load or verify set.\n");
			return err;
		}
		else
			err = VWB_ERROR_NONE;

		VWB_Warper_base* p = (VWB_Warper_base*)pWarper;
		err = p->Init(set);
		DeleteVWF(set);
	}	
	return err;
}

void VWB_Destroy( VWB_Warper* pWarper )
{
#ifdef WIN32
	if( !g_listeners.empty() )
	{
		for( std::vector< SPtr< VWBTCPListener > >::iterator it = g_listeners.begin(); it != g_listeners.end(); it++ )
		{
			if( VWB_ERROR_NONE == (*it)->remove( pWarper ) )
			{
				if( (*it)->empty() )
				{
					g_server.removeReceiver( SPtr<SockIn>(*it) );
					g_listeners.erase( it );
					if( g_server.empty() )
						g_server.endModal(0);
				}
				break;
			}
		}
	}
#endif def WIN32
	if( pWarper )
		delete (VWB_Warper_base*)pWarper;
}

VWB_ERROR VWB_getViewProj( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj )
{

	if( NULL != pEye && NULL != pRot )
	{
		logStr( 4, "VWB_getViewProj\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper )
	{
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewProjection( pEye, pRot, pView, pProj );
		if( NULL != pView && NULL != pProj )
		{
			logStr( 4,
					" OUT: view: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"      proj: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					, pView[0], pView[1], pView[2], pView[3]
					, pView[4], pView[5], pView[6], pView[7]
					, pView[8], pView[9], pView[10], pView[11]
					, pView[12], pView[13], pView[14], pView[15]
					, pProj[0], pProj[1], pProj[2], pProj[3]
					, pProj[4], pProj[5], pProj[6], pProj[7]
					, pProj[8], pProj[9], pProj[10], pProj[11]
					, pProj[12], pProj[13], pProj[14], pProj[15]
			);
		}
		return err;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getViewClip( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip )
{
	if( NULL != pEye && NULL != pRot )
	{
		logStr( 4, "VWB_getViewClip\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper )
	{
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewClip( pEye, pRot, pView, pClip );
		if( NULL != pView && NULL != pClip )
		{
			logStr( 4,
					" OUT: view: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"      clip: (%f, %f, %f, %f, %f, %f)\n"
					, pView[0], pView[1], pView[2], pView[3]
					, pView[4], pView[5], pView[6], pView[7]
					, pView[8], pView[9], pView[10], pView[11]
					, pView[12], pView[13], pView[14], pView[15]
					, pClip[0], pClip[1], pClip[2], pClip[3], pClip[4], pClip[5]
			);
		}
		return err;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_setViewProj( VWB_Warper* pWarper, VWB_float* pView,  VWB_float* pProj )
{
	if( pWarper )
		return ((VWB_Warper_base*)pWarper)->SetViewProjection( pView, pProj );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_render( VWB_Warper* pWarper, VWB_param inputTexture, VWB_uint restoreMask )
{
	logStr( 4, "VWB_Render" );
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;
	VWB_ERROR err = ((VWB_Warper_base*)pWarper)->Render( inputTexture, restoreMask );
	return err;
}

VWB_ERROR VWB_vwfInfo( char const* path, VWB_WarpBlendHeaderSet* set )
{
	return ScanVWF( path, set );
}
VWB_ERROR VWB__logString( VWB_int level, char const* str )
{
	if( NULL == str || 0 == str[0] )
		return VWB_ERROR_PARAMETER;
	logStr( level, str );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_getWarpBlendMesh( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	if( pWarper )
		return ((VWB_Warper_base*)pWarper)->getWarpMesh( cols, rows, mesh );
	return VWB_ERROR_PARAMETER;
}

#include "mmath.h"

VWB_Warper_base::VWB_Warper_base()
: m_type4cc( 0 )
, m_sizeMap( _size0 )
, m_sizeIn( _size0 )
, m_bDynamicEye( false )
, m_bBorder( false )
, m_mBaseI( VWB_MAT44f::I() )
, m_mViewIG( VWB_MAT44f::I() )
, m_bRH(true)
, m_mVP( VWB_MAT44f::I() )
, m_hmEPP( 0 )
, m_hEPP( NULL )
, m_fnEPPCreate( NULL )
, m_fnEPPGet( NULL )
, m_fnEPPRelease( NULL )
{
	Defaults();
	memset( &m_ep, 0, sizeof( m_ep ) );
	m_viewSizes.x = 1;
	m_viewSizes.y = 1;
	m_viewSizes.z = 1;
	m_viewSizes.w = 1;
	m_blackBias.x = 0;
	m_blackBias.y = 0;
	m_blackBias.z = 0;
	m_blackBias.w = 0;
}

VWB_Warper_base::~VWB_Warper_base()
{
	if( m_hmEPP )
	{
		if( m_hEPP && m_fnEPPRelease )
			m_fnEPPRelease( m_hEPP );
        #ifdef WIN32
		::FreeLibrary( m_hmEPP );
        #endif
	}
}

VWB_ERROR VWB_Warper_base::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR ret = VWB_ERROR_NONE;
	// determine screen
	if( 0 > calibIndex )
	{
		int iScreen = -calibIndex;
		char szScreen[18];
		sprintf_s( szScreen, "D%u ", iScreen );
		int l = (int)strlen( szScreen );
		int i = 0;
		for( ; i != (int)wbs.size(); i++ )
		{
			char const* si = wbs[i]->header.name;
			if( 0 == strncmp( si, szScreen, l ) )
			{
				logStr( 2, "AdapterOrdinal %d (%s) resolved to calibIndex %d.\n", -calibIndex, si, i );
				calibIndex = i;
				break;
			}
		}
		if( i == (int)wbs.size() )
		{
			logStr( 0, "ERROR: Could not find mapping of display D%u.", iScreen );
			return VWB_ERROR_GENERIC;
		}
	}

#ifdef WIN32
	ShowSystemCursor = reinterpret_cast<FPtrInt_BOOL>(::GetProcAddress(::GetModuleHandle("user32.dll"), "ShowSystemCursor"));
	if (nullptr == ShowSystemCursor)
	{
		logStr(1, "WARNING: Could not find ShowSystemCursor function.");
	}
#endif //def WIN32


	if( (VWB_int)wbs.size() <= calibIndex )
	{
		logStr( 0, "ERROR: calibIndex out of range. There is(are) only %u dataset(s).", (VWB_uint)wbs.size() );
		return VWB_ERROR_GENERIC;
	}

	VWB_WarpBlend& wb = *wbs[calibIndex];

    m_bBorder = 0 != ( FLAG_WARPFILE_HEADER_BORDER & wbs[calibIndex]->header.flags );
	m_bDynamicEye = 0 != ( FLAG_WARPFILE_HEADER_3D & wbs[calibIndex]->header.flags );
	if( m_bDynamicEye )
		logStr( 2, "3D data found in mapping. Using DYNAMIC EYE settings.\n" );
	else
		logStr( 2, "2D data found in mapping. Using FIXED EYE settings.\n" );

	m_sizeMap.cx = wbs[calibIndex]->header.width;
	m_sizeMap.cy = wbs[calibIndex]->header.height;
	m_blackBias.x = wbs[calibIndex]->header.black[0];
	m_blackBias.y = wbs[calibIndex]->header.black[1];
	m_blackBias.z = wbs[calibIndex]->header.black[2];
	m_blackBias.w = wbs[calibIndex]->header.black[3];

	VWB_MAT44f B( trans );
	m_mBaseI = B.Inverted();
    VWB_VECTOR3<float>(B.X()) * VWB_VECTOR3<float>(B.Y());
	m_bRH = 0 < VWB_VEC3f(B.Z()).dot( VWB_VEC3f(B.X()) * VWB_VEC3f(B.Y()) );
	logStr( 2, "%s-handedness detected.\n", m_bRH ? "right" : "left" );

	// sanity check
	if( 1024 > m_sizeMap.cx * m_sizeMap.cx )
	{
		logStr( 0, "ERROR: mapping too small. Probably a faulty mapping file.\n" );
		return VWB_ERROR_VWF_LOAD;
	}

	if( NULL == wb.pWarp )
	{
		logStr( 0, "ERROR: Warp texture missing. Seriously.\n" );
		return VWB_ERROR_WARP;
	}

	if( NULL == wb.pBlend )
	{
		logStr( 1, "WARNING: Blend texture missing. Creating...\n" );
		wb.pBlend2 = new VWB_BlendRecord2[m_sizeMap.cx * m_sizeMap.cy];
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++ )
		{
			p->r = 65535;
			p->g = 65535;
			p->b = 65535;
		}
		wb.header.flags&= ~FLAG_WARPFILE_HEADER_BLENDV3;
		wb.header.flags|=  FLAG_WARPFILE_HEADER_BLENDV2;
	}

	// regardless of the given format, prepare blend as U16 NORM
	// if gamma is set, apply gamma and promote blend to VWB_BlendRecord2
	if( 0.0f < gamma && 1.0f != gamma )
	{
		logStr( 1, "Adapting gamma by %.5f\n", gamma );
		VWB_float g = 1.0f/gamma;
		if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 )
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[m_sizeMap.cx * m_sizeMap.cy];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord3* p = wb.pBlend3, *pE = wb.pBlend3 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( pow( p->r, g ) * 65535.0f );
				pD->g = VWB_word( pow( p->g, g ) * 65535.0f );
				pD->b = VWB_word( pow( p->b, g ) * 65535.0f );
				pD->a = VWB_word( p->a * 65535.0f );
			}
			wb.header.flags&= ~FLAG_WARPFILE_HEADER_BLENDV3;
			wb.header.flags|= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend3;
			wb.pBlend2 = pDst;
		}
		else if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2 )
		{
			for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++ )
			{
				p->r = VWB_word( pow( VWB_float( p->r ) / 65535.0f, g ) * 65535.0f );
				p->g = VWB_word( pow( VWB_float( p->g ) / 65535.0f, g ) * 65535.0f );
				p->b = VWB_word( pow( VWB_float( p->b ) / 65535.0f, g ) * 65535.0f );
				// p->a stays untouched
			}
		}
		else
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[m_sizeMap.cx * m_sizeMap.cy];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord* p = wb.pBlend, *pE = wb.pBlend + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( pow( VWB_float( p->r ) / 255.0f, g ) * 65535.0f );
				pD->g = VWB_word( pow( VWB_float( p->g ) / 255.0f, g ) * 65535.0f );
				pD->b = VWB_word( pow( VWB_float( p->b ) / 255.0f, g ) * 65535.0f );
				pD->a = VWB_word( p->a ) * 255;
			}
			wb.header.flags|= FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend;
			wb.pBlend2 = pDst;
		}
	}
	else if( !(wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2) )
	{ // change to VWB_BlendRecord2 anyway...
		if( wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 )
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[m_sizeMap.cx * m_sizeMap.cy];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord3* p = wb.pBlend3, *pE = wb.pBlend3 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( p->r * 65535.0f );
				pD->g = VWB_word( p->g * 65535.0f );
				pD->b = VWB_word( p->b * 65535.0f );
				pD->a = VWB_word( p->a * 65535.0f );
			}
			wb.header.flags&= ~FLAG_WARPFILE_HEADER_BLENDV3;
			wb.header.flags|=  FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend3;
			wb.pBlend2 = pDst;
		}
		else
		{
			VWB_BlendRecord2* pDst = new VWB_BlendRecord2[m_sizeMap.cx * m_sizeMap.cy];
			VWB_BlendRecord2* pD = pDst;
			for( VWB_BlendRecord* p = wb.pBlend, *pE = wb.pBlend + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pD++ )
			{
				pD->r = VWB_word( p->r * 255.0f );
				pD->g = VWB_word( p->g * 255.0f );
				pD->b = VWB_word( p->b * 255.0f );
				pD->a = VWB_word( p->a * 255.0f );
			}
			wb.header.flags|=  FLAG_WARPFILE_HEADER_BLENDV2;
			delete[] wb.pBlend;
			wb.pBlend2 = pDst;
		}
	}

	// put clipping channel of warp to a channel of blend
		VWB_WarpRecord* pW = wb.pWarp;
	if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
	{
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pW++ )
		{
			p->a = pW->w * 65535.0f;
		}
	}
	else
	{
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pW++ )
		{
			p->a = pW->z * 65535.0f;
		}
	}

	if( 0 == ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) )
	{
		// in case of 2D mapping
		// calculate bounds, init with opposite extremum
		wb.header.vPartialCnt[0] = 1; // left
		wb.header.vPartialCnt[1] = 1; // top
		wb.header.vPartialCnt[2] = 0; // right
		wb.header.vPartialCnt[3] = 0; // bottom
		VWB_BlendRecord2* pB = wb.pBlend2;
		for( VWB_WarpRecord* pW = wb.pWarp, *pWE = wb.pWarp + m_sizeMap.cx * m_sizeMap.cy; pW != pWE; pW++, pB++ )
		{
			if(0.5f <= pW->z && ( 0 < pB->r || 0 < pB->g || 0 < pB->b))
			{
				if (wb.header.vPartialCnt[0] > pW->x)
					wb.header.vPartialCnt[0] = pW->x;
				if (wb.header.vPartialCnt[1] > pW->y)
					wb.header.vPartialCnt[1] = pW->y;
				if (wb.header.vPartialCnt[2] < pW->x)
					wb.header.vPartialCnt[2] = pW->x;
				if (wb.header.vPartialCnt[3] < pW->y)
					wb.header.vPartialCnt[3] = pW->y;
			}
		}
		if (wb.header.vPartialCnt[2] <= wb.header.vPartialCnt[0] ||
			wb.header.vPartialCnt[3] <= wb.header.vPartialCnt[1])
		{
			wb.header.vPartialCnt[0] = 0;
			wb.header.vPartialCnt[1] = 0;
			wb.header.vPartialCnt[2] = 0;
			wb.header.vPartialCnt[3] = 0;
			logStr( 1, "WARNING: Could not calculate content bounds." );
		}
		else
		{
			wb.header.vPartialCnt[4] = (wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0]) / (wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1]);
		}
	}

	// initialize eye provider, in case it is stated
	if( eyeProvider[0] )
	{
		char p[MAX_PATH] = {0};
		strcpy( p, eyeProvider );
#ifdef WIN32
		m_hmEPP = LoadLibraryA( p );
		if( 0 == m_hmEPP )
		{
			logStr( 0, "ERROR: Could not find eye point provider module: \"%s\".\n", p );
			return VWB_ERROR_PARAMETER;
		}
		m_fnEPPCreate = (pfn_CreateEyePointReceiver)GetProcAddress( m_hmEPP, "CreateEyePointReceiver" );
		m_fnEPPGet = (pfn_ReceiveEyePoint)GetProcAddress( m_hmEPP, "ReceiveEyePoint" );
		m_fnEPPRelease  = (pfn_DeleteEyePointReceiver)GetProcAddress( m_hmEPP, "DeleteEyePointReceiver" );
		if( NULL == m_fnEPPCreate || NULL == m_fnEPPGet || NULL == m_fnEPPRelease )
		{
			logStr( 0, "ERROR: Could not load eye point provider methods from \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
#else
//#error implement!
		logStr( 0, "ERROR: Not implemented. Could not load eye point provider methods from \"%s\".\n", p );
		return VWB_ERROR_NOT_IMPLEMENTED;
#endif //def WIN32

		m_hEPP = m_fnEPPCreate( eyeProviderParam );
		if( NULL == m_hEPP )
		{
			logStr( 0, "ERROR: Could not initialize eye point provider \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
		logStr( 1, "Eye point provider \"%s\" successfully initialized.\n", p );
	}

	// do auto calculations
	VWB_MAT44f S = VWB_MAT44f::I(); // scaling matrix
	if( m_bDynamicEye )
	{
		if( bAutoView )
		{
			VWB_ERROR res = AutoView( *wbs[calibIndex] );		// viewport calculation
			if( VWB_ERROR_NONE != res )
				logStr( 0, "ERROR: Autoview returns error %d.\n", res );
		}	
		else
		{
			optimalRes.cx = wb.header.width;
			optimalRes.cy = wb.header.height;
			optimalRect.left =	0;
			optimalRect.top =	0;
			optimalRect.right = wb.header.width;
			optimalRect.bottom =wb.header.height;
		}
		// collect dimensions
		VWB_BOXf b = VWB_BOXf::M();
		VWB_WarpRecord* pW = wb.pWarp;
		for( VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + m_sizeMap.cx * m_sizeMap.cy; p != pE; p++, pW++ )
		{
			if( 0.5 <= pW->w )
			{
				b += VWB_VEC3f::ptr( &pW->x );
			}
		}

		// scale and shift to avoid clamping
		S = VWB_MAT44f::S(1.0f/(b.br.x - b.tl.x), 1.0f/(b.br.y - b.tl.y), 1.0f/(b.br.z - b.tl.z), 1 ) * VWB_MAT44f::T( -b.tl.x, -b.tl.y, -b.tl.z );
		pW = wb.pWarp;
		VWB_VEC3f a,c;
		for( VWB_WarpRecord const* pWE = pW + m_sizeMap.cx * m_sizeMap.cy; pW != pWE; pW++ )
		{
			if( 0.5 < pW->w )
			{
				a = VWB_VEC3f::ptr( &pW->x );
				VWB_VEC3f::ptr( &pW->x ) = S * a;
				c = S.Inverted() * VWB_VEC3f::ptr( &pW->x );
			}
		}

	}
	else
	{
		if( bAutoView )
		{
			if( 0 != wb.header.vCntDispPx[4] && 0 != wb.header.vCntDispPx[5] )
			{
				optimalRes.cx = (VWB_int)(1.0f / wb.header.vCntDispPx[4]);
				optimalRes.cy = (VWB_int)(1.0f / wb.header.vCntDispPx[5]);
			}
			else
			{
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal resolution due to missing information in warp file\n" );
			}
			if( 0 != wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0] &&
				0 != wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1] )
			{
				optimalRect.left =	(VWB_int)(wb.header.vPartialCnt[0] / wb.header.vCntDispPx[4]);
				optimalRect.top =	(VWB_int)(wb.header.vPartialCnt[1] / wb.header.vCntDispPx[5]);
				optimalRect.right = (VWB_int)(wb.header.vPartialCnt[2] / wb.header.vCntDispPx[4]);
				optimalRect.bottom =(VWB_int)(wb.header.vPartialCnt[3] / wb.header.vCntDispPx[5]);
			}
			else
			{
				optimalRect.left =	0;
				optimalRect.top =	0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom =wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal partial rect due to missing information in warp file\n" );
			}
			logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
				"optimalRes=[%d, %d]\n"
				"optimalRect=[%d, %d, %d, %d]\n",
				channel,
				optimalRes.cx, optimalRes.cy,
				optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom );
		}
		else
		{
			optimalRes.cx = wb.header.width;
			optimalRes.cy = wb.header.height;
			optimalRect.left =	0;
			optimalRect.top =	0;
			optimalRect.right = wb.header.width;
			optimalRect.bottom =wb.header.height;
		}
	}

	// translate world to local IG's coordinates
	// translation/rotation to VIOSO's eye point is done via base matrix later

	// we have to inverse rotate and translate
	m_mViewIG = m_bRH ? VWB_MAT44f::R( DEG2RAD(dir[0]), DEG2RAD(dir[1]), DEG2RAD(dir[2]) ).Transposed() : VWB_MAT44f::RLHT( DEG2RAD(dir[0]), DEG2RAD(dir[1]), DEG2RAD(dir[2]) );

	// scale and shift back to original, to cancel out clipping scale/offset
	m_mBaseI*= S.Inverted();

	// Remind the sizes of the frustum on screenDist
	m_viewSizes.x = tan( DEG2RAD(fov[0] ) ) * screenDist; // left
	m_viewSizes.y = tan( DEG2RAD(fov[1] ) ) * screenDist; // top
	m_viewSizes.z = tan( DEG2RAD(fov[2] ) ) * screenDist; // right
	m_viewSizes.w = tan( DEG2RAD(fov[3] ) ) * screenDist; // bottom

	void* hTmp = m_hEPP;
	m_hEPP = NULL;
	GetViewProjection( NULL, NULL, NULL, NULL );
	m_hEPP = hTmp;

	return ret;
}

VWB_ERROR VWB_Warper_base::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	// receive the current car parameters
	::memset( &m_ep, 0, sizeof( m_ep ) );
	if( m_hEPP && m_fnEPPGet ) // try eye porvider
	{
		m_fnEPPGet( m_hEPP, &m_ep );
		if( eye )
		{
			eye[0] = (float)m_ep.x;
			eye[1] = (float)m_ep.y;
			eye[2] = (float)m_ep.z;
		}
		if( rot )
		{
			rot[0] = (float)m_ep.pitch;
			rot[1] = (float)m_ep.yaw;
			rot[2] = (float)m_ep.roll;
		}

		logStr( 3, "INFO: EyePointReceiver %s input for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
				eyeProviderParam, channel,
				m_ep.x, m_ep.y, m_ep.z,
				m_ep.pitch, m_ep.yaw, m_ep.roll );
	}
	else
	{ // use given
		if( eye )
			spliceVec( m_ep.x, m_ep.y, m_ep.z, eye[0], eye[1], eye[2], ( splice >> 16 ) );

		if( rot )
			spliceVec( m_ep.pitch, m_ep.yaw, m_ep.roll, rot[0], rot[1], rot[2], splice );

		if( NULL != eye && NULL != rot )
			logStr( 3, "INFO: Eyepoint from call for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
					channel,
					eye[0], eye[1], eye[2],
					rot[0], rot[1], rot[2] );
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
{
	// receive the current car parameters
	::memset( &m_ep, 0, sizeof( m_ep ) );
	if( m_hEPP && m_fnEPPGet ) // try eye porvider
	{
		m_fnEPPGet( m_hEPP, &m_ep );
		if( eye )
		{
			eye[0] = (float)m_ep.x;
			eye[1] = (float)m_ep.y;
			eye[2] = (float)m_ep.z;
		}
		if( rot )
		{
			rot[0] = (float)m_ep.pitch;
			rot[1] = (float)m_ep.yaw;
			rot[2] = (float)m_ep.roll;
		}

		logStr( 3, "INFO: EyePointReceiver %s input for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
				eyeProviderParam, channel,
				m_ep.x, m_ep.y, m_ep.z,
				m_ep.pitch, m_ep.yaw, m_ep.roll );
	}
	else
	{ // use given
		if( eye )
			spliceVec( m_ep.x, m_ep.y, m_ep.z, eye[0], eye[1], eye[2], ( splice >> 16 ) );

		if( rot )
			spliceVec( m_ep.pitch, m_ep.yaw, m_ep.roll, rot[0], rot[1], rot[2], splice );

		if( NULL != eye && NULL != rot )
			logStr( 3, "INFO: Eyepoint from call for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
					channel,
					eye[0], eye[1], eye[2],
					rot[0], rot[1], rot[2] );
	}

	return VWB_ERROR_NONE;
}

void VWB_Warper_base::getClip( VWB_VEC3f const& e, VWB_float * pClip )
{
	//VWB_float dd = nearDist / ( screenDist -e.z );
	VWB_float dd = nearDist / ( screenDist + ( m_bRH ? e.z : -e.z ) );
	pClip[0] = ( m_viewSizes[0] + e.x ) * dd;
	pClip[1] = ( m_viewSizes[1] + e.y ) * dd;
	pClip[2] = ( m_viewSizes[2] - e.x ) * dd;
	pClip[3] = ( m_viewSizes[3] - e.y ) * dd;
	pClip[4] = nearDist;
	pClip[5] = farDist;
}


void VWB_Warper_base::Defaults()
{
	size_t sz1 = sizeof( VWB_Warper );
	size_t sz2 = sizeof( VWB_Warper_base );
	::memset( &this->path, 0, sz1 );
	strcpy( calibFile, "vioso.vwf" );
	strcpy( channel, "channel 1" );
	nearDist = 0.1f;
	farDist = 200.0f;
	fov[0]=30;
	fov[1]=20;
	fov[2]=30;
	fov[3]=20;
	trans[0] = 1;
	trans[5] = 1;
	trans[10] = 1;
	trans[15] = 1;
	screenDist = 1;
	autoViewC = 1;
}

VWB_ERROR VWB_Warper_base::AutoView( VWB_WarpBlend const& wb )
{
	// test if blend3
	if( !( FLAG_WARPFILE_HEADER_BLENDV2 & wb.header.flags ) )
		return VWB_ERROR_PARAMETER;

	// check base matrix, if left or right handed...
	// global base matrix
	VWB_MAT44d B = VWB_MAT44f::ptr(trans);
	VWB_MAT44d Bi = B.Inverted();

// find x and y axis from scan
	// find corners
	VWB_WarpRecord* pW = wb.pWarp;
	VWB_BlendRecord2* pB = wb.pBlend2;
	int wh = wb.header.width / 2;
	int hh = wb.header.height / 2;
	VWB_WarpRecord *ptl = NULL, *ptr = NULL, *pbl = NULL, *pbr = NULL; // the extremal corners
	int ltl = INT_MAX, ltr = INT_MIN, lbl = INT_MAX, lbr = INT_MIN; // the extremal corner's distance to projector centre on projector
	for( int y = hh - wb.header.height; y != hh; y++ )
	{
		for( int x = -wh; x != wb.header.width - wh; x++, pW++, pB++ )
		{
			if( 1 == pW->w && 
				0 != pB->a &&
				0 != ( pB->r + pB->g + pB->b ) )
			{
				//int sq = x * x + y * y;
				int d1 = x + y;
				int d2 = x - y;

				// top left
				if( d1 < ltl )
				{
					ltl = d1;
					ptl = pW;
				}

				// top right
				if( d2 > ltr )
				{
					ltr = d2;
					ptr = pW;
				}

				// bottom left
				if( d2 < lbl )
				{
					lbl = d2;
					pbl = pW;
				}

				//bottom right
				if( d1 > lbr )
				{
					lbr = d1;
					pbr = pW;
				}
			}
		}
	}


	//TODO reduce the chance of picking an outlier by using corrected average.
	// but the chance of an outlier is really low, because the data is adjusted by the Calibrator
	if( NULL == ptl || NULL == ptr || NULL == pbl || NULL == pbr )
	{
		logStr( 1, "WARINIG: AutoView cannot find corners of screen.\n" );
		return VWB_ERROR_GENERIC;
	}
	else
	{
		logStr( 2, "INFO: AutoView mapping display corners (%.4f,%.4f,%.4f)-(%.4f,%.4f,%.4f).\n", ptl->x,ptl->y,ptl->z,pbr->x,pbr->y,pbr->z );
	}

	// use corners to calculate 
	// use mid points and transform to IG coordinates
	VWB_VEC3d dx,dy,dtl,dtr,dbl,dbr; // the main coordinate axes to-be.

	dtl = VWB_VEC3f::ptr((VWB_float*)ptl);
	dtr = VWB_VEC3f::ptr((VWB_float*)ptr);
	dbl = VWB_VEC3f::ptr((VWB_float*)pbl);
	dbr = VWB_VEC3f::ptr((VWB_float*)pbr);
	dtl = Bi * dtl;
	dtr = Bi * dtr;
	dbr = Bi * dbr;
	dbl = Bi * dbl;

	dx = dtr + dbr - dtl - dbl;
	dy = dtl + dtr - dbl - dbr;
	
	// calculate local base matrix to IG coordinates
	VWB_MAT33d M = VWB_MAT33d::base( dx, dy );
	VWB_MAT44d T = VWB_MAT44d(M) * Bi;

	VWB_VEC3d c = ( dtl + dtr + dbl + dbr ) / 4; // the centre in IG
	VWB_VEC3d cL = M * c; // centre in local coordinates
	screenDist = VWB_float( cL.z ); // eye to plane distance

	// getting rotation angles from local coords in global 
	if( m_bRH )
	{
		VWB_VEC3f::ptr(dir) = M.Transposed().GetR_RH() * ( 180.0 / M_PI );
	}
	else
	{
		VWB_VEC3f::ptr(dir) = M.GetR_LHT() * ( 180.0 / M_PI );
	}

	
	// caclulate FoVs
	double minDx = FLT_MAX, minDy = FLT_MAX;  // minimal horizontal and vertical projected distance on render plane, for quality purposes
	double maxEL = FLT_MAX, maxET = FLT_MAX, maxER = -FLT_MAX, maxEB = -FLT_MAX;  // maximum horizontal and vertical view size, left top right bottom throughout moving space

	// now we get the corners of a box in that coordinate system
	VWB_BOXd b( VWB_VEC3d(DBL_MAX, DBL_MAX, DBL_MAX), VWB_VEC3d(-DBL_MAX, -DBL_MAX, -DBL_MAX) );

	pW = wb.pWarp;
	pB = wb.pBlend2;
	double cnt = 0;

	double l = autoViewC * abs(screenDist) / 4;

	VWB_VEC4d* pTLB = new VWB_VEC4d[wb.header.width+1]; // transformed of previous value and prevoius line, to calculate projected distance
	VWB_VEC4d* pTL = pTLB, *pTLE = pTLB + (wb.header.width + 1);
	for( pTL = pTLB; pTL != pTLE; pTL++ )
		pTL->w = 0;
	for( int y = 0; y != wb.header.height; y++ )
	{
		pTLB->w = 0;
		pTL = pTLB + 1;
		for( int x = 0; x != wb.header.width; x++, pW++, pB++, pTL++ )
		{
			if( 1 == pW->w && 
				0 != pB->a &&
				0 != ( pB->r + pB->g + pB->b ) )
			{
				VWB_VEC3d v( pW->x, pW->y, pW->z);
				VWB_VEC3d vTT = T * v;
				b+= vTT;

				// project v to the screen plane
				// this yields the normal view plane size
				double vx = vTT.x / vTT.z;
				double vy = vTT.y / vTT.z;
				// widen by movement
				double dd = abs( l * ( vTT.z - screenDist ) / vTT.z );
				if( maxEL > vx - dd )
					maxEL = vx - dd;
				if( maxET > vy - dd )
					maxET = vy - dd;
				if( maxER < vx + dd)
					maxER = vx + dd;
				if( maxEB < vy + dd)
					maxEB = vy + dd;

				// find smallest distance between 2 neighbouring points
				if( 1 == pTLB->w )
				{ // previous value valid, go for horizontal
					VWB_double d = vTT.x - pTLB->x;
					if( minDx > d )
						minDx = d;
				}
				if( 1 == pTL->w )
				{ // upper value is valid
					VWB_double d = vTT.y - pTL->y;
					if( minDy > d )
						minDy = d;
				}
				*pTL = vTT; // w is set to 1 by implicit assignment through constructor
				*pTLB = vTT;
			}
			else
				pTL->w = 0;

		}

	}
	delete[] pTLB;

	if( FLT_MAX == maxEL || FLT_MAX == maxET || -FLT_MAX == maxER || -FLT_MAX == maxEB )
	{
		logStr( 1, "WARNING: AutoView cannot calculate FoVs.\n" );
		return VWB_ERROR_GENERIC;
	}

	// now we have a minimal box around our point cloud, but in local base coordinates
	// now we transform IG's eye to that coordinate system

	double left =	b.tl.x;
	double top =	b.tl.y;
	double right =	b.br.x;
	double bottom = b.br.y;
	screenDist = abs( screenDist );

	fov[0] = VWB_float( RAD2DEG( atan(-maxEL) ) );
	fov[1] = VWB_float( RAD2DEG( atan(-maxET) ) );
	fov[2] = VWB_float( RAD2DEG( atan( maxER) ) );
	fov[3] = VWB_float( RAD2DEG( atan (maxEB) ) );
	VWB_VEC4d borderFit( maxEL / left * screenDist, maxET / top * screenDist, maxER / right * screenDist, maxEB / bottom * screenDist );

	optimalRes.cx = abs( VWB_int( ( maxER - maxEL ) * screenDist / minDx ) );
	optimalRes.cy = abs( VWB_int( ( maxEB - maxET ) * screenDist / minDy ) );
	optimalRect.left = 0;
	optimalRect.top = 0;
	optimalRect.right = optimalRes.cx;
	optimalRect.bottom = optimalRes.cy;
	logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
		"dir=[%.6f, %.6f, %.6f]\n"
		"fov=[%.6f, %.6f, %.6f, %.6f]\n"
		"screenDist=%.6f\n"
		"optimalRes=[%d, %d]\n"
		"optimalRect=[%d, %d, %d, %d]\n"
		"handedness=%s\n"
		"borderFitError=[%.6f%%, %.6f%%, %.6f%%, %.6f%%]\n",
		channel,
		dir[0],dir[1],dir[2],
		fov[0],fov[1],fov[2],fov[3],
		screenDist,
		optimalRes.cx, optimalRes.cy,
		optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom,
		m_bRH ? "right" : "left",
		(borderFit.x-1)*100,(borderFit.y-1)*100,(borderFit.z-1)*100,(borderFit.w-1)*100 );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	if( 0 != port )
	{
		for( auto listener = g_listeners.begin(); listener != g_listeners.end(); listener++ )
		{
			if( this == listener->ptr->getWarper( channel ) )
			{
				VWBTCPConnection* conn = dynamic_cast<VWBTCPConnection*>( listener->ptr );
				if( conn )
				{
					HttpRequest const* req = conn->headRequestPtr();
					if( nullptr != req )
					{
						HttpRequest::PostParamMap::const_iterator it = req->postData.find( "inifile" );
						if( it != req->postData.end() && !it->second.file.empty() )
						{
							it->second.file;
							// TODO ovrewrite current ini file
						}
						it = req->postData.find( "jasonrpc" );
						if( it != req->postData.end() && !it->second.value.empty() )
						{
							it->second.value;
							// TODO ovrewrite current ini file
						}
					}
				}
			}
		}
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////

Dummywarper::Dummywarper(void)
	: VWB_Warper_base()
{
	memset( &m_wb, 0, sizeof( m_wb ) );
	m_type4cc = 'MMUD';
}

Dummywarper::~Dummywarper(void)
{
	DeleteVWF( m_wb );
}

VWB_ERROR Dummywarper::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init(wbs);
	if( VWB_ERROR_NONE == err )
	{
		// do a deep copy of the mappings, to keep
		m_wb = *wbs[calibIndex];
		int nRecords = m_wb.header.width * m_wb.header.height;
		if( m_wb.pWarp )
		{
			m_wb.pWarp = new VWB_WarpRecord[nRecords];
			memcpy( m_wb.pWarp, wbs[calibIndex]->pWarp, nRecords * sizeof( VWB_WarpRecord ) );
		}
		if( m_wb.pBlend2 )
		{
			m_wb.pBlend2 = new VWB_BlendRecord2[nRecords];
			memcpy( m_wb.pBlend2, wbs[calibIndex]->pBlend2, nRecords* sizeof( VWB_BlendRecord2 ) );
		}
		if( m_wb.pBlack )
		{
			m_wb.pBlack = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pBlack, wbs[calibIndex]->pBlack, nRecords * sizeof( VWB_BlendRecord ) );
		}
		if( m_wb.pWhite )
		{
			m_wb.pWhite = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pWhite, wbs[calibIndex]->pWhite, nRecords * sizeof( VWB_BlendRecord ) );
		}
	}
	return err;
}

VWB_ERROR Dummywarper::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	VWB_ERROR ret = VWB_Warper_base::GetViewProjection( eye, rot, pView, pProj );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f V; // the view matrix to return 
		VWB_MAT44f P; // the projection matrix to return 

		// we transform the dynamic eye-point to the constant view:

		// copy eye coordinate to e
		VWB_VEC3f e( (float)m_ep.x, (float)m_ep.y, (float)m_ep.z );

		// rotation matrix from angles
		VWB_MAT44f R = m_bRH ? VWB_MAT44f::R( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll ).Transposed() : VWB_MAT44f::RLHT( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );

		// add eye offset rotated to platform
		if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
			e += VWB_VEC3f::ptr( this->eye ) * R;

		// translate to local coordinates
		e = e * m_mViewIG;

		VWB_MAT44f T = VWB_MAT44f::TT( -e );
		m_mVP = m_mBaseI * m_mViewIG * T; //TODO precalc

		if( bTurnWithView )
			V = R * m_mViewIG;
		else
			V = m_mViewIG * T;

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::PRHT( m_viewSizes, nearDist, farDist, screenDist, e );
		else
			P = VWB_MAT44f::PLHT( m_viewSizes, nearDist, farDist, screenDist, e );

		m_mVP *= P;

		if( pView )
		{
			V.SetPtr( pView );
		}

		if( pProj )
		{
			P.SetPtr( pProj );
		}
	}
	return ret;
}

VWB_ERROR Dummywarper::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
{
	VWB_ERROR ret = VWB_Warper_base::GetViewClip( eye, rot, pView, pClip );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f V; // the view matrix to return 
		VWB_MAT44f P; // the projection matrix to return 

		// we transform the dynamic eye-point to the constant view:

		// copy eye coordinate to e
		VWB_VEC3f e( (float)m_ep.x, (float)m_ep.y, (float)m_ep.z );

		// rotation matrix from angles
		VWB_MAT44f R = m_bRH ? VWB_MAT44f::R( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll ).Transposed() : VWB_MAT44f::RLHT( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );

		// add eye offset rotated to platform
		if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
			e += VWB_VEC3f::ptr( this->eye ) * R;

		// translate to local coordinates
		e = e * m_mViewIG;

		VWB_MAT44f T = VWB_MAT44f::TT( -e );
		m_mVP = m_mBaseI * m_mViewIG * T; //TODO precalc

		if( bTurnWithView )
			V = R * m_mViewIG;
		else
			V = m_mViewIG * T;

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::PRHT( m_viewSizes, nearDist, farDist, screenDist, e );
		else
			P = VWB_MAT44f::PLHT( m_viewSizes, nearDist, farDist, screenDist, e );

		m_mVP *= P;

		if( pView )
		{
			V.SetPtr( pView );
		}

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}
	return ret;
}

VWB_ERROR Dummywarper::SetViewProjection( VWB_float const* pView, VWB_float const* pProj )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR Dummywarper::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

_inline_ VWB_uint getTrianglePatch( 
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 )
{
	// find uv coordinate range
	VWB_uint d1 = dx1 + out.header.width * dy1;
	VWB_uint d2 = dx2 + out.header.width * dy2;
	VWB_WarpRecord* pW1 = pW + d1;
	VWB_WarpRecord* pW2 = pW + d2;
	VWB_float uMax = pW->x, uMin = pW->x;
	VWB_float vMax = pW->y, vMin = pW->y;
	if( uMax < pW1->x )
		uMax = pW1->x;
	if( uMin > pW1->x )
		uMin = pW1->x;
	if( uMax < pW2->x )
		uMax = pW2->x;
	if( uMin > pW2->x )
		uMin = pW2->x;

	if( vMax < pW1->y )
		vMax = pW1->y;
	if( vMin > pW1->y )
		vMin = pW1->y;
	if( vMax < pW2->y )
		vMax = pW2->y;
	if( vMin > pW2->y )
		vMin = pW2->y;
	uMin = floor( uMin * out.header.width  ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width  ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ )
	{
		for( VWB_float u = uMin; u <= uMax; u++ )
		{
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f(u/out.header.width,v/out.header.height,1), l ) )
			{
				VWB_uint o = VWB_int(u)+out.header.width*VWB_int(v);
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x,y,1 ), VWB_VEC3f( x+dx1,y+dy1,1 ), VWB_VEC3f( x+dx2,y+dy2,1 ), l, VWB_VEC3f::ptr(&out.pWarp[o].x) );
				if( pB && out.pBlend2 )
				{
					out.pBlend2[o].r = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->r + l.y * ( pB + d1 )->r + l.z * ( pB + d2 )->r ));
					out.pBlend2[o].g = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->g + l.y * ( pB + d1 )->g + l.z * ( pB + d2 )->g ));
					out.pBlend2[o].b = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->b + l.y * ( pB + d1 )->b + l.z * ( pB + d2 )->b ));
					out.pBlend2[o].a = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->a + l.y * ( pB + d1 )->a + l.z * ( pB + d2 )->a ));
				}
				if( pBl && out.pBlack )
				{
					out.pBlack[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->r + l.y * (pBl + d1)->r + l.z * (pBl + d2)->r ) );
					out.pBlack[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->g + l.y * (pBl + d1)->g + l.z * (pBl + d2)->g ) );
					out.pBlack[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->b + l.y * (pBl + d1)->b + l.z * (pBl + d2)->b ) );
					out.pBlack[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->a + l.y * (pBl + d1)->a + l.z * (pBl + d2)->a ) );
				}				
				if( pWh && out.pWhite )
				{				
					out.pWhite[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->r + l.y * (pWh + d1)->r + l.z * (pWh + d2)->r ) );
					out.pWhite[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->g + l.y * (pWh + d1)->g + l.z * (pWh + d2)->g ) );
					out.pWhite[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->b + l.y * (pWh + d1)->b + l.z * (pWh + d2)->b ) );
					out.pWhite[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->a + l.y * (pWh + d1)->a + l.z * (pWh + d2)->a ) );
				}
				conv++;
			}
		}
	}
	return conv;
}

_inline_ VWB_uint getTrianglePatchA( 
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 )
{
	// find uv coordinate range
	VWB_uint d1 = dx1 + out.header.width * dy1;
	VWB_uint d2 = dx2 + out.header.width * dy2;
	VWB_WarpRecord* pW1 = pW + d1;
	VWB_WarpRecord* pW2 = pW + d2;
	VWB_float uMax = pW->x, uMin = pW->x;
	VWB_float vMax = pW->y, vMin = pW->y;
	if( uMax < pW1->x )
		uMax = pW1->x;
	if( uMin > pW1->x )
		uMin = pW1->x;
	if( uMax < pW2->x )
		uMax = pW2->x;
	if( uMin > pW2->x )
		uMin = pW2->x;

	if( vMax < pW1->y )
		vMax = pW1->y;
	if( vMin > pW1->y )
		vMin = pW1->y;
	if( vMax < pW2->y )
		vMax = pW2->y;
	if( vMin > pW2->y )
		vMin = pW2->y;
	uMin = floor( uMin * out.header.width  ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width  ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	if( 5 < uMax - uMin || 5 < vMax - vMin )
	{
		int i = 0;
	}
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ )
	{
		for( VWB_float u = uMin; u <= uMax; u++ )
		{
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f(u/out.header.width,v/out.header.height,1), l ) )
			{
				VWB_uint o = VWB_int(u)+out.header.width*VWB_int(v);
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x,y,1 ), VWB_VEC3f( x+1,y,1 ), VWB_VEC3f( x+1,y+1,1 ), l, VWB_VEC3f::ptr(&out.pWarp[o].x) );
				out.pBlend2[o].r = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->r + l.y * ( pB + d1 )->r + l.z * ( pB + d2 )->r ) );
				out.pBlend2[o].g = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->g + l.y * ( pB + d1 )->g + l.z * ( pB + d2 )->g ) );
				out.pBlend2[o].b = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->b + l.y * ( pB + d1 )->b + l.z * ( pB + d2 )->b ) );
				out.pBlend2[o].a = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->a + l.y * ( pB + d1 )->a + l.z * ( pB + d2 )->a ) );
				conv++;
			}
		}
	}
	return conv;
}

VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out )
{
	logStr( 2, "INFO: Calculate inverse maping of %s.\n", in.header.name );
	VWB_int const& w = in.header.width;
	VWB_int const& h = in.header.height;
	VWB_int const nRecords = w * h;

	out = in;
	if( in.pWarp )
	{
		out.pWarp = new VWB_WarpRecord[nRecords];
		memset( out.pWarp, 0, nRecords * sizeof( VWB_WarpRecord ) );
	}
	else
		return VWB_ERROR_PARAMETER;

	if( out.pBlend2 )
	{
		out.pBlend2 = new VWB_BlendRecord2[nRecords];
		memset( out.pBlend2, 0, nRecords* sizeof( VWB_BlendRecord2 ) );
	}
	if( out.pBlack )
	{
		out.pBlack = new VWB_BlendRecord[nRecords];
		memset( out.pBlack, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}
	if( out.pWhite )
	{
		out.pWhite = new VWB_BlendRecord[nRecords];
		memset( out.pWhite, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}

	// fill from triangles...
	VWB_uint conv = 0;
	VWB_WarpRecord   *pW  = in.pWarp  ;
	VWB_BlendRecord2 *pB  = in.pBlend2;
	VWB_BlendRecord* pBl = in.pBlack;
	VWB_BlendRecord* pWh = in.pWhite;
	VWB_WarpRecord const* pWE = pW + nRecords - w;


// if there was no triangle on the left, try point at lower left
// P5    P0
//     / | 
//    /  | 
//   / A | 
// P4----P3
//
// normal patch, P1 optional
// P0---(P1)
// | \ C |
// |  \  |
// | B \ |
// P3----P2
//
// if P2 is missing triangulate other way
// P0----P1
// |    /
// | D /
// |  /
// P3
//
	if( out.pBlack || out.pWhite || !out.pBlend2 )
	{
		for( VWB_float y = 0; pWE != pW; pW++, pB++, pBl++, pWh++, y++ )
		{
			VWB_WarpRecord const *pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, pBl++, pWh++, x++ )
			{
				if( 0.5f < pW->z )
				{
					VWB_WarpRecord* pW3 = pW+w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > (pW-1)->z &&
						0.5f < pW3->z )
					{
						VWB_WarpRecord* pW4 = pW+w-1;   // bottom-left
						if( 0.5f < pW4->z )
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW+w+1; // bottomright
					VWB_WarpRecord* pW1 = pW+1; // right
					if( 0.5f < pW2->z )
					{
						if( 0.5f < pW1->z ) // Triangle A
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					}
					else
					{
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}
				}
			}
		}
	}
	else
	{
		for( VWB_float y = 0; pWE != pW; pW++, pB++, y++ )
		{
			VWB_WarpRecord const *pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, x++ )
			{
				if( 0.5f < pW->z )
				{
					VWB_WarpRecord* pW3 = pW+w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > (pW-1)->z &&
						0.5f < pW3->z )
					{
						VWB_WarpRecord* pW4 = pW+w-1;   // bottom-left
						if( 0.5f < pW4->z )
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW+w+1; // bottomright
					VWB_WarpRecord* pW1 = pW+1; // right
					if( 0.5f < pW2->z )
					{
						if( 0.5f < pW1->z ) // Triangle A
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					}
					else
					{
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}

				}
			}
		}
	}

	// fill holes
	VWB_WarpRecord   *pOutW  = out.pWarp   + w + 1;
	VWB_BlendRecord2 *pOutB  = out.pBlend2 + w + 1;
	VWB_BlendRecord  *pOutBl = out.pBlack  + w + 1;
	VWB_BlendRecord  *pOutWh = out.pWhite  + w + 1;
	for( VWB_WarpRecord const* pOutWE = pOutW + nRecords - 2 * w; pOutWE != pOutW; pOutW+= 2, pOutB+= 2, pOutBl+= 2, pOutWh+= 2 )
	{
		for( VWB_WarpRecord const* pOutWLE = pOutW + w - 2; pOutWLE != pOutW; pOutW++, pOutB++, pOutBl++, pOutWh++ )
		{
			if( 0.5f > pOutW->z )
			{
				VWB_WarpRecord* pW1 = pOutW-1; // left
				VWB_WarpRecord* pW2 = pOutW+1; // right
				VWB_WarpRecord* pW3 = pOutW-w; // top
				VWB_WarpRecord* pW4 = pOutW+w; // bottom
				if( 0.5f < pW1->z &&
					0.5f < pW2->z )
				{
					pOutW->x = ( pW1->x + pW2->x ) / 2;
					pOutW->y = ( pW1->y + pW2->y ) / 2;
					pOutW->z = ( pW1->z + pW2->z ) / 2;
					if( out.pBlend2 )
					{
						pOutB->r = (VWB_word)(( (VWB_int)(pOutB-1)->r + (VWB_int)(pOutB+1)->r ) / 2);
						pOutB->g = (VWB_word)(( (VWB_int)(pOutB-1)->g + (VWB_int)(pOutB+1)->g ) / 2);
						pOutB->b = (VWB_word)(( (VWB_int)(pOutB-1)->b + (VWB_int)(pOutB+1)->b ) / 2);
						pOutB->a = (VWB_word)(( (VWB_int)(pOutB-1)->a + (VWB_int)(pOutB+1)->a ) / 2);
					}
					conv++;
				}
				else if( 0.5f < pW3->z &&
					0.5f < pW4->z )
				{
					pOutW->x = ( pW3->x + pW4->x ) / 2;
					pOutW->y = ( pW3->y + pW4->y ) / 2;
					pOutW->z = ( pW3->z + pW4->z ) / 2;
					if( out.pBlend2 )
					{
						pOutB->r = (VWB_word)( ( (VWB_int)( pOutB - w )->r + (VWB_int)( pOutB + w )->r ) / 2 );
						pOutB->g = (VWB_word)( ( (VWB_int)( pOutB - w )->g + (VWB_int)( pOutB + w )->g ) / 2 );
						pOutB->b = (VWB_word)( ( (VWB_int)( pOutB - w )->b + (VWB_int)( pOutB + w )->b ) / 2 );
						pOutB->a = (VWB_word)( ( (VWB_int)( pOutB - w )->a + (VWB_int)( pOutB + w )->a ) / 2 );
					}
					conv++;
				}
			}
		}
	}
	if( 64 <= conv )
		return VWB_ERROR_NONE;
	else
		return VWB_ERROR_FALSE;
}

/*
VWB_uint subMesh( VWB_WarpBlendMesh::idx_t& idx, VWB_WarpBlendMesh::idx_t& oldRef, VWB_WarpBlendMesh::idx_t& oldTrail, VWB_WarpBlendMesh::idx_t& newIdx, VWB_WarpBlendMesh::idx_t& newRef, VWB_WarpBlendMesh::idx_t& newTrail )
{
	if( 3 > oldRef.size() )
		return 0;

	//newIdx.clear();
	//newRef.clear();
	//newTrail.clear();
	newIdx.push_back( idx[oldRef[0] + oldTrail[0] ] ); 
	newIdx.push_back( idx[oldRef[0] + oldTrail[1] ] ); 
	newIdx.push_back( idx[oldRef[1] + oldTrail[2] ] ); 
	newRef.push_back( 0 );
	newTrail.push_back( 0 );
	newTrail.push_back( 2 );

	VWB_uint sz = (VWB_uint)oldTrail.size();
	VWB_uint sz3 = sz-2;
	for( VWB_uint i = 2; i < sz3; i++ )
	{
		newRef.push_back( i * 3 );
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newTrail.push_back( 2 );
	}
	newRef.push_back( sz * 3 );
	newIdx.push_back( idx[oldRef[sz-3] + oldTrail[sz-2] ] ); 
	newIdx.push_back( idx[oldRef[sz-2] + oldTrail[sz-1] ] ); 
	newIdx.push_back( idx[oldRef[0] + oldTrail[0] ] ); 

	if( 3 < sz )
	{
		VWB_WarpBlendMesh::idx_t nnIdx;
		VWB_WarpBlendMesh::idx_t nnRef;
		VWB_WarpBlendMesh::idx_t nnTrail;
		subMesh( newIdx, newRef, newTrail, nnIdx, nnRef, nnTrail ); 
	}
	return 0;
}
*/
	typedef enum ESPUMA_CtrlPointUseFlag
	{
		FLAG_CTRLPT_USE_UNSPECIFIC				=0x00,							///<   unspecific using
		FLAG_CTRLPT_USE_SELECTED					=0x01,							///<   point is selected
		FLAG_CTRLPT_USE_SELECTED_TANGENT			=0x02,							///<   a tangent to point is selected
		FLAG_CTRLPT_USE_SELECTED_TANGENT_SECOND	=0x04,							///<   second tangent point is selected
		FLAG_CTRLPT_USE_SELECTED_LINE_RIGHT		=0x08,							///<   line to the right is selected
		FLAG_CTRLPT_USE_SELECTED_LINE_BOTTOM		=0x10,							///<   line down is selected
		FLAG_CTRLPT_USE_SELECTED_ALL				=0x1F
	}ESPUMA_CtrlPointUseFlag;

	typedef enum ESPUMA_CtrlPointPosFlag
	{
		FLAG_CTRLPT_POS_NORMAL=0x0,												///<   normal control point
		FLAG_CTRLPT_POS_UNKNOWN=0x1,												///<   indicates an control point with unknown position
		FLAG_CTRLPT_POS_BORDER_TOP=0x2,											///<   indicates that the control point is located on the top border of the control point mesh
		FLAG_CTRLPT_POS_BORDER_LEFT=0x4,											///<   indicates that the control point is located on the left border of the control point mesh
		FLAG_CTRLPT_POS_BORDER_RIGHT=0x8,										///<   indicates that the control point is located on the right border of the control point mesh
		FLAG_CTRLPT_POS_BORDER_BOTTOM=0x10,										///<   indicates that the control point is located on the bottom border of the control point mesh
		FLAG_CTRLPT_POS_BORDER_DIAGONAL=0x20,									///<   indicates that the control point is located on the diagonal border of the control point mesh
		FLAG_CTRLPT_POS_CLONE=0x40,												///<   indicates that the control point is only a clone of another point
		FLAG_CTRLPT_POS_BORDER= FLAG_CTRLPT_POS_BORDER_TOP |					///<   all border flags
								   FLAG_CTRLPT_POS_BORDER_LEFT | 
								   FLAG_CTRLPT_POS_BORDER_RIGHT | 
								   FLAG_CTRLPT_POS_BORDER_BOTTOM | 
								   FLAG_CTRLPT_POS_BORDER_DIAGONAL,
		FLAG_CTRLPT_POS_BORDER_LR= FLAG_CTRLPT_POS_BORDER_LEFT | 				///<   right and left border
									  FLAG_CTRLPT_POS_BORDER_RIGHT,
		FLAG_CTRLPT_POS_TOPLEFT=		FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top left, indicates the origin of the mesh
										FLAG_CTRLPT_POS_BORDER_LEFT,
		FLAG_CTRLPT_POS_TOPRIGHT=	FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top right
										FLAG_CTRLPT_POS_BORDER_RIGHT,
		FLAG_CTRLPT_POS_BOTTOMLEFT=	FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom left
										FLAG_CTRLPT_POS_BORDER_LEFT,
		FLAG_CTRLPT_POS_BOTTOMRIGHT= FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom right
										FLAG_CTRLPT_POS_BORDER_RIGHT
	}ESPUMA_CtrlPointPosFlag;

	typedef struct SPPair3f
	{
		int                                 fUse;									///<   common flag to indicate special using of the point
																					///<  @see ESPUMA_CtrlPointUseFlag
		int                                 fPos;									///<   flag to describe the position of the point in a mesh, if any
																					///<  @see ESPUMA_CtrlPointPosFlag
		float                               lPt1[3];								///<   first point pair
		float                               lPt2[3];								///<   second point pair
		float                               lTangDescX[2];							///<   parameter list to descibe an optional tangent of the point for extrapolation in x direction ([0] distance, [1] degree)
		float                               lTangDescY[2];							///<   parameter list to descibe an optional tangent of the point for extrapolation in y direction ([0] distance, [1] degree)
		void Empty()
		{
			::memset( this, 0, sizeof( SPPair3f ) );
			lTangDescX[0]=1.0f;
			lTangDescY[0]=1.0f;
		}
	} SPPair3f;

	typedef std::vector<SPPair3f> DynSPPointPairList3f;
	typedef std::vector<long> DynLongList;
	typedef std::vector<long*> DynLongPtrList;
	typedef std::vector<unsigned int> DynDWORDList;

int ComputeTriangluation( DynSPPointPairList3f& lPoints, DynLongList& lTriangleIdx, VWB_WarpRecord* pSrcD, VWB_BlendRecord3* pSrcDB, long width, long height, unsigned int qConsolidateSteps )
{
	lPoints.clear();
	lTriangleIdx.clear();

	if(!( pSrcD && (width>0) && (height>0) ) )
		return 0;
	if( (width==1) || (height==1) )
		return 1;

	int f;
	float yF;
	SPPair3f* pP;
	long i,j,k,idx;
	VWB_WarpRecord* pW, *pW1;
	VWB_BlendRecord3* pB;
	std::vector<long> IdxD;

	idx=width*height;
	IdxD.resize( idx, 0 );
	size_t q;
	long *pL,*pL1,*pL2;
	DynLongPtrList lLnPtr;
	DynDWORDList lPtsPerLn;
	unsigned int q1,q2,qH,qV,qHMin,qRgnH,qRgnV,qRgn;

	if(qConsolidateSteps)
	{
		lLnPtr.reserve((size_t)height);
		lPtsPerLn.reserve((size_t)height);
	}

	lPoints.reserve((size_t)idx);

	idx*=6;
	lTriangleIdx.reserve((size_t)idx);

	pL=&IdxD[0];
	pW=pSrcD;
	pB=pSrcDB;

	long w = width - 1;
	long h = height - 1;

	// translate to map where lPt1 ist the vertex coordinate and lPt2 is the texture coordinate; fill the index list
	for( idx=0, i=0; i<=h ; i++)
	{
		for( yF=(float)i, j=0; j<=w ; j++, pW++, pL++, pB++ )
		{
			if( pW->z > 0.5f ) // test if valid
			{
				SPPair3f d;
				lPoints.push_back(d);
				pP = &lPoints.back();
				pP->Empty();
				pP->lPt2[0] = pW->x;
				pP->lPt2[1] = pW->y;
				pP->lPt1[0] = (float)j;
				pP->lPt1[1] = yF;
				pP->lTangDescX[0]= pB->r;
				pP->lTangDescX[1]= pB->g;
				pP->lTangDescY[0]= pB->b;
				pP->lTangDescY[1]= pB->a;
				if(i<h) // look below
				{
					pW1 = pW + width;
					if( pW1->z > 0.5f ) // point is valid
					{
						pP->fPos|=FLAG_CTRLPT_POS_BORDER_BOTTOM;
						pP->fUse++;
					}
				}
				else
				{
					pW1=NULL;
				}
				if(j<w)
				{
					if(pW1) 
					{
						pW1++; // look below right
						if( pW1->z > 0.5f ) // valid
						{
							pP->fPos|=FLAG_CTRLPT_POS_BORDER_DIAGONAL;
							pP->fUse++;
						}
					}
					pW1 = pW + 1; // look right
					if( pW1->z > 0.5f )
					{
						pP->fPos|=FLAG_CTRLPT_POS_BORDER_RIGHT;
						pP->fUse++;
					}
				}
				*pL=idx; // set index
				idx++;
			}
			else
			{
				*pL=-1; // mark index as invalid
			}
		}
	}

	pL = &IdxD[0];
	for( i=0; i<h ; i++, pL++)
	{
		for( j=0; j<w ; j++, pL++)
		{
			idx=*pL;
			if(idx<0)
				continue;
			pP=&lPoints[idx];
			k=pP->fUse;
			if(k<=1)
				continue; // no neighboars

			pL2=pL+width; // this is the point below
			if(k==2)
			{
				// add one triangle
				q = lTriangleIdx.size();
				lTriangleIdx.insert( lTriangleIdx.end(), 3, 0 );
				pL1=&lTriangleIdx[q];
				pL1[0]=idx;
				f=pP->fPos;
				if(f & FLAG_CTRLPT_POS_BORDER_DIAGONAL)
				{
					if(f & FLAG_CTRLPT_POS_BORDER_RIGHT)
					{
						pL1[1]=pL[1];
						pL1[2]=pL2[1];
					}
					else
					{
						pL1[1]=pL2[1];
						pL1[2]=pL2[0];
					}
				}
				else
				{
					pL1[1]=pL[1];
					pL1[2]=pL2[0];
				}
			}
			else if(qConsolidateSteps)
			{
				lLnPtr.clear();
				lPtsPerLn.clear();
				
				// find maximum rect smaller than step x step with all valid points

				// go from current point down
				qHMin = width; // initialize with maximum
				for( pL1=pL, qV=0; qV<=qConsolidateSteps ; qV++, pL1+=width)
				{
					// go from current point right
					for( pL2=pL1, qH=0; qH<=qConsolidateSteps ; qH++, pL2++)
					{
						if(!(lPoints[*pL2].fPos & FLAG_CTRLPT_POS_BORDER_RIGHT))
						{ // point has no right neighboar
							qH++;
							break;
						}
					}
					if(qH==1) // no valid points at right, break outer loop because we reached minimum
					{
						qHMin = 1;
						break;
					}

					lLnPtr.push_back(pL1); // remind position pointer in that line
					lPtsPerLn.push_back(qH); // remind number of points gone right in that line

					// assign 
					if(qH<qHMin)
						qHMin=qH;

					// break loop if no valid bottom point is there
					if(!(lPoints[*pL1].fPos & FLAG_CTRLPT_POS_BORDER_BOTTOM))
					{
						break;
					}
				}

				qH=lPtsPerLn[0];
				qRgnV=(unsigned int)lLnPtr.size();
				qRgnH=qHMin;
				qRgn=qRgnH*qRgnV;

				// try make a bigger rect with less lines
				for( q1=1; q1<qV ; q1++)
					if(lPtsPerLn[q1]<qH)
						break;

				if(q1==1)
				{
					q2=2*lPtsPerLn[1];
					if(q2>qRgn)
					{
						qRgnV=2;
						qRgnH=lPtsPerLn[1];
						qRgn=q2;
					}
				}
				else
				{
					q2=q1*qH;
					if(q2>qRgn) // take other rect
					{
						qRgnV=q1;
						qRgnH=qH;
						qRgn=q2;
					}
				}

				// look for the biggest square
				qHMin=MIN( lPtsPerLn[0], lPtsPerLn[1]);
				for( q1=2; q1<qV ; q1++)
				{
					q2=lPtsPerLn[q1];
					if(q2<qHMin)
						qHMin=q2;
					if(qHMin<=q1)
						break;
				}
				// look for the biggest square
				if(qHMin<q1)
					q1=qHMin;
				q2=q1*q1;
				if(q2>qRgn)
				{
					qRgnV=q1;
					qRgnH=q1;
					qRgn=q2;
				}

				qRgnH--;
				qRgnV--;
				pL2=lLnPtr[qRgnV]+qRgnH;

				// save the triangles' indices
				q = lTriangleIdx.size();
				lTriangleIdx.insert( lTriangleIdx.end(), 6, 0 );
				pL1=&lTriangleIdx[q];
				pL1[0]=idx;
				pL1[1]=pL[qRgnH];
				pL1[2]=*pL2;
				pL1[3]=idx;
				pL1[4]=*pL2;
				pL1[5]=*(lLnPtr[qRgnV]);

				// mark points as unused
				for( qV=0; qV<qRgnV ; qV++)
					for( pL1=lLnPtr[qV], qH=0; qH<qRgnH ; qH++, pL1++)
						if( 0 <= *pL1 )
							lPoints[*pL1].fUse=0;
			}
			else
			{
				// save the triangles' indices
				q = lTriangleIdx.size();
				lTriangleIdx.insert( lTriangleIdx.end(), 6, 0 );
				pL1=&lTriangleIdx[q];
				pL1[0]=idx;
				pL1[1]=pL[1];
				pL1[2]=pL2[1];
				pL1[3]=idx;
				pL1[4]=pL2[1];
				pL1[5]=pL2[0];
			}
		}
	}
	return 1;
}

VWB_ERROR Dummywarper::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	if( 3 > cols || 3 > rows )
	{
		logStr( 0, "ERROR: getWarpMesh needs >3 rows and columns.\n" );
		return VWB_ERROR_PARAMETER;
	}

	if( m_bDynamicEye )
	{
		logStr( 0, "ERROR: getWarpMesh cannot deal with dynamic eye yet.\n" );
		return VWB_ERROR_NOT_IMPLEMENTED;
	}

	VWB_int& w = m_wb.header.width;
	VWB_int& h = m_wb.header.height;
	int nRecords = w * h;

	if( 1024 > nRecords || NULL == m_wb.pWarp || NULL == m_wb.pBlend )
	{
		logStr( 0, "ERROR: getWarpMesh: warp map too small.\n" );
		return VWB_ERROR_GENERIC;
	}
	logStr( 2, "INFO: getWarpMesh: params OK.\n" );

	//VWB_WarpBlend wbInv;
	//VWB_ERROR err = invertWB( m_wb, wbInv );
	//if( VWB_ERROR_NONE == err )
	{
		//logStr( 2, "INFO: getWarpMesh: inverted.\n" );

		DynSPPointPairList3f points;
		DynLongList	indices;
//		if( ComputeTriangluation( points, indices, wbInv.pWarp, wbInv.pBlend3, w, h, (DWORD)sqrt( (float)w * h / 20000 ) ) )
		if( ComputeTriangluation( points, indices, m_wb.pWarp, m_wb.pBlend3, w, h, (unsigned int)sqrt( (float)w * h / 20000 ) ) )
		{
			logStr( 2, "INFO: getWarpMesh: triangulation.\n" );
			DynLongList indTrans; indTrans.resize( points.size(), -1 );
			mesh.nIdx = (VWB_uint)indices.size();
			mesh.idx = new VWB_uint[ mesh.nIdx ];
	
			VWB_uint nRes = mesh.nIdx / 2;
			mesh.vtx = new VWB_WarpBlendVertex[nRes];
			mesh.nVtx = 0;

			mesh.dim.cx = w;
			mesh.dim.cy = h;

			DynLongList::iterator iSrc = indices.begin();
			logStr( 2, "INFO: getWarpMesh: resize.\n" );
			for( VWB_uint* iDst = mesh.idx, *iDstE = iDst+mesh.nIdx; iDst != iDstE; iDst++, iSrc++ )
			{
				long const& i = *iSrc;
				long& j = indTrans[i];
				if( -1 == j )
				{
					if( mesh.nVtx >= nRes )
					{
						VWB_uint n = nRes;
						nRes*= 2;
						VWB_WarpBlendVertex* t = mesh.vtx;
						mesh.vtx = new VWB_WarpBlendVertex[nRes];
						memcpy( mesh.vtx, t, n * sizeof( VWB_WarpBlendVertex ) );
						delete[] t;
					}
					j = mesh.nVtx;
					VWB_WarpBlendVertex const v = {
						{points[i].lPt1[0] / w, points[i].lPt1[1] / h ,0},
						{points[i].lPt2[0],points[i].lPt2[1]},
						{points[i].lTangDescX[0],points[i].lTangDescX[1],points[i].lTangDescY[0]}
					};
					mesh.vtx[mesh.nVtx] = v;
					mesh.nVtx++;
				}
				*iDst = (VWB_uint)j;
			}
			logStr( 2, "INFO: getWarpMesh: Triangulation succeeded. %u vertices with %u triangles.\n", mesh.nVtx, mesh.nIdx / 3 );

#ifdef _DEBUG
			for( VWB_uint i = 0; i != mesh.nIdx; i+= 3 )
			{
				VWB_WarpBlendVertex tri[3] = { mesh.vtx[mesh.idx[i]], mesh.vtx[mesh.idx[i+1]], mesh.vtx[mesh.idx[i+2]] };
				if( 
					!( 0< tri[0].pos[0] && tri[0].pos[0] < 1 ) &&
					!( 0< tri[0].pos[1] && tri[0].pos[1] < 1 ) ||
					!( 0< tri[1].pos[0] && tri[1].pos[0] < 1 ) &&
					!( 0< tri[1].pos[1] && tri[1].pos[1] < 1 ) ||
					!( 0< tri[2].pos[0] && tri[2].pos[0] < 1 ) &&
					!( 0< tri[2].pos[1] && tri[2].pos[1] < 1 )
					)
				{
					int j = 0;
				}
			}

			struct sdm_vertex
			{
				VWB_float _vtx[2];    // 
				VWB_word  _uv[2];    // normalized ushort2
				VWB_uint _alpha;   // 
			};

			sdm_vertex*  vtx = new sdm_vertex[mesh.nVtx];
			for (VWB_uint i = 0; i != mesh.nVtx; i++)
			{
				vtx[i]._vtx[0] = mesh.vtx[i].pos[0];
				vtx[i]._vtx[1] = mesh.vtx[i].pos[1];
				vtx[i]._uv[0] = (VWB_word)MIN(65535, MAX(0, mesh.vtx[i].uv[0] * 65535));
				vtx[i]._uv[1] = (VWB_word)MIN(65535, MAX(0, mesh.vtx[i].uv[1] * 65535));
				vtx[i]._alpha = (VWB_uint)MAX(0, (mesh.vtx[i].rgb[0] + mesh.vtx[i].rgb[1] + mesh.vtx[i].rgb[2]) / 3.0f * 4294967295L);
			}

			VWB_uint* idx = new VWB_uint[mesh.nIdx];
			for (VWB_uint i = 0; i != mesh.nIdx; i++)
				idx[i] = mesh.idx[i];

#endif
		}
		else
		{
			logStr( 0, "ERROR: getWarpMesh: Triangulation (CT) failed.\n" );
			return VWB_ERROR_GENERIC;
		}
		return VWB_ERROR_NONE;
		
#if 0
	// triangulate points

		mesh.idx.clear();
		mesh.vtx.clear();
		mesh.dim.cx = w;
		mesh.dim.cy = h;
		typedef std::vector< VWB_WarpBlendMesh::idx_t > RefList;
		RefList refs; refs.resize(nRecords);
		
		mesh.idx.reserve( nRecords * 2 );
		mesh.vtx.reserve( nRecords );
		VWB_WarpRecord   *pW  = wbInv.pWarp  ;
		VWB_BlendRecord2 *pB  = wbInv.pBlend2;
		VWB_WarpRecord const* pWE = pW + nRecords - w;


						//i3 = newIndex[o+w];
						//if( -1 == i3 )
						//{
						//	VWB_BlendRecord2* pB3 = pB+w;
						//	VWB_WarpBlendVertex const v3 = { {pW3->x, pW3->y, pW3->z}, {VWB_float(x),VWB_float(y+1)}, {pB3->r, pB3->g, pB3->b} };
						//	i3 = (VWB_uint)mesh.vtx.size();
						//	mesh.vtx.push_back( v3 );
						//	newIndex[o+w] = i3;
						//}
								//		VWB_BlendRecord2* pB1 = pB+1;
								//VWB_WarpBlendVertex const v1 = { {pW1->x, pW1->y, pW1->z}, {VWB_float(x+1),VWB_float(y)}, {pB1->r, pB1->g, pB1->b} };
								//i1 = (VWB_uint)mesh.vtx.size();
								//mesh.vtx.push_back( v1 );
								//newIndex[o+1] = i2;

//       P0----P1
//     / | \ A |
//    /  |  \  |
//   / C | B \ |
// P4----P3----P2

//       P0----P1
//     / |    /
//    /  | D /
//   / C |  /
// P4----P3
//

		std::vector< VWB_int > fixed; fixed.resize( nRecords, 0 );
		for( VWB_uint y = 0, o = 0; pWE != pW; pW++, pB++, y++, o++ )
		{
			VWB_WarpRecord const *pWLE = pW + w - 1;
			for( VWB_uint x = 0; pWLE != pW; pW++, pB++, x++, o++ )
			{
				if( 0.5f < pW->z )
				{
					VWB_WarpRecord* pW3 = pW+w;   // bottom
					VWB_WarpRecord* pW2 = pW+w+1; // bottomright
					VWB_WarpRecord* pW1 = pW+1; // right
					if( 0.5f < pW2->z )
					{
						if( 0.5f < pW1->z ) // Triangle A
						{
							VWB_uint iTri = (VWB_uint)mesh.idx.size();
							mesh.idx.push_back( o );
							mesh.idx.push_back( o+1 );
							mesh.idx.push_back( o+w+1 );
							refs[o].push_back( iTri );
							refs[o+1].push_back( iTri );
							refs[o+w+1].push_back( iTri );
						}

						if( 0.5f < pW3->z )  // Triangle B
						{
							VWB_uint iTri = (VWB_uint)mesh.idx.size();
							mesh.idx.push_back( o );
							mesh.idx.push_back( o+w+1 );
							mesh.idx.push_back( o+w );
							refs[o].push_back( iTri );
							refs[o+w+1].push_back( iTri );
							refs[o+w].push_back( iTri );
						}
					}
					else
					{
						if( 0.5f < pW1->z ) // Triangle D
						{
							VWB_uint iTri = (VWB_uint)mesh.idx.size();
							mesh.idx.push_back( o );
							mesh.idx.push_back( o+1 );
							mesh.idx.push_back( o+w );
							refs[o].push_back( iTri );
							refs[o+1].push_back( iTri );
							refs[o+w].push_back( iTri );
						}
					}

					// add extra triangle, if previous point is invalid and therefore not added in previous step
					VWB_WarpRecord* pW4 = pW+w-1; // bottom-left
					if( pW4 > pWLE &&   // Triangle C
						0.5f < pW3->z &&
						0.5f < pW4->z &&
						0.5f > (pW-1)->z )
					{
						VWB_uint iTri = (VWB_uint)mesh.idx.size();
						mesh.idx.push_back( o );
						mesh.idx.push_back( o+w );
						mesh.idx.push_back( o+w-1 );
						refs[o].push_back( iTri );
						refs[o+w].push_back( iTri );
						refs[o+w-1].push_back( iTri );
					}
				}
				else
					fixed[o] = 3; // point not available
			}
		}
		// thinning
		// we connect the trail of left-over triangle edges
		VWB_WarpBlendMesh::idx_t trail; trail.reserve( 40 );

		for( VWB_uint i = 0; i != nRecords; i++ )
		{
			if( 0 == fixed[i] )
			{
				RefList::value_type& l = refs[i];
				VWB_uint sz = (VWB_uint)l.size();
				if( 3 <= sz )
				{
					VWB_uint j;
					VWB_float x0 = wbInv.pWarp[i].x;
					VWB_float y0 = wbInv.pWarp[i].y;
					VWB_word r0 = wbInv.pBlend2[i].r;
					VWB_word g0 = wbInv.pBlend2[i].g;
					VWB_word b0 = wbInv.pBlend2[i].b;
					do
					{
AGAIN:
						// sort edges
						j = l[0]; // this is the first triangle in index list
						//push in the triangle index
						trail.clear();
						// then the vertex offset in that triangle of the dangeling end of the remaining triangle edge
						// we always go clockwise, so this keeps beeing that way
						if( i == mesh.idx[j] ) // we picked point itself, so 
						{
							trail.push_back(1); // second point is start
							trail.push_back(2); // third point is dangeling end
						}
						else if( i == mesh.idx[j+1] ) // the next point clockwise is itself
						{
							trail.push_back(2); // last point is start
							trail.push_back(0); // and first point dangles
						}
						else
						{
							trail.push_back(0); // first point is start
							trail.push_back(1); // second point dangles otherwise
						}
						VWB_int lastDangelingEnd = mesh.idx[j+trail.back()];
						for( VWB_uint pushed = 1; pushed != sz; )
						{
							for( VWB_uint k = pushed; ; )
							{
								// we look for a triangle containing the same point as lastDangelingEnd
								VWB_uint n = l[pushed];
								for( VWB_int m = 0; m != 3; m++ )
								{
									if( lastDangelingEnd == mesh.idx[n+m] )
									{ // found
										if( 2 == m )
											trail.push_back( 0 );
										else
											trail.push_back( ++m );
										pushed++;
										lastDangelingEnd = mesh.idx[n+m];
										goto DONE;
									}
								}
								// swap and try next
								k++;
								if( k == sz )
									break;
								VWB_uint tmp = l[pushed];
								l[pushed] = l[k];
								l[k] = tmp;
							}
							// nothing found; we have an open trail
							// swap last with first triangle, and start over again
							// maybe find better criteria... 
							VWB_uint tmp = l[sz-1];
							l[sz-1] = l[0];
							l[0] = tmp;
							goto AGAIN;
DONE:	
							(NULL);
						}
					}while(0);

					// we can remove last edge from trail if it closes
					VWB_uint fr = mesh.idx[l.front()+trail.front()];
					VWB_uint ba = mesh.idx[l.back()+trail.back()];
					if( fr == ba )
						trail.pop_back();

					// build new submesh
					VWB_WarpBlendMesh::idx_t newL;
					VWB_WarpBlendMesh::idx_t newTrail;
					VWB_WarpBlendMesh::idx_t newIdx;

					subMesh( mesh.idx, l, trail, newIdx, newL, newTrail );
				}
				else
					fixed[i] = 1;
			}
		}
#endif
		//DeleteVWF( wbInv );
	}
	//else
	//	logStr( 0, "ERROR: getWarpMesh: invertWB failed.\n" ); 
	return VWB_ERROR_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////


inline void spliceVec( VWB_double& outX, VWB_double& outY, VWB_double& outZ, VWB_double const& inX, VWB_double const& inY, VWB_double const& inZ, VWB_uint sw )
{
	if( sw & 0x002 )
		outX = inY;
	else if( sw & 0x004 )
		outX = inZ;
	else
		outX = inX;
	if( sw & 0x001 )
		outX*= -1;

	if( sw & 0x020 )
		outY = inX;
	else if( sw & 0x040 )
		outY = inZ;
	else
		outY = inY;
	if( sw & 0x010 )
		outY*= -1;

	if( sw & 0x200 )
		outZ = inX;
	else if( sw & 0x400 )
		outZ = inY;
	else
		outZ = inZ;
	if( sw & 0x100 )
		outZ*= -1;

}

#ifdef WIN32
size_t copyCursorBitmapToMappedTexture( HBITMAP hbmMask, HBITMAP hbmColor, BITMAP& bmMask, BITMAP& bmColor,  void* bits, int pitch )
{
	size_t r = 0;

	LONG szMask = bmMask.bmWidthBytes * abs( bmMask.bmHeight );

	bmMask.bmBits = malloc( szMask );
	BYTE* pData = reinterpret_cast<BYTE*>(malloc( abs( bmMask.bmHeight ) * pitch ));
	BYTE* pMask = reinterpret_cast<BYTE*>(malloc( szMask ));
	BYTE* pM = pMask;
	r = ::GetBitmapBits( hbmMask, szMask, pMask );
	if( hbmColor )
	{
		LONG szColor = bmColor.bmWidthBytes * abs( bmColor.bmHeight );
		r = ::GetBitmapBits( hbmColor, szColor, pData );
	}
	else
	{
		bmColor.bmWidth = bmMask.bmWidth;
		bmColor.bmWidthBytes = bmMask.bmWidth * 4;
		bmColor.bmHeight = bmMask.bmHeight / 2;
		bmColor.bmBitsPixel = 32;
		bmColor.bmPlanes = bmMask.bmPlanes;

		BYTE* data = pData;
		for( int y = 0; y != abs( bmMask.bmHeight / 2 ); y++, pM += bmMask.bmWidthBytes, data += pitch )
		{
			BYTE* byteAND = pM;
			BYTE maskAND = *byteAND;
			BYTE* byteXOR = pM + (bmMask.bmHeight * bmMask.bmWidthBytes / 2);
			BYTE maskXOR = *byteXOR;
			BYTE* pixel = data;

			for( int x = 0; x != bmMask.bmWidth; x++, pixel += 4 )
			{
				if( maskXOR & maskAND & 128 ) // -> reverse screen ??, we take green
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				}
				else if( maskXOR & 128 ) // -> white
				{
					pixel[0] = 255;
					pixel[1] = 255;
					pixel[2] = 255;
					pixel[3] = 255;
				}
				else if( maskAND & 128 ) // -> screen
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 0;
				}
				else // -> black
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				}

				if( 7 == x % 8 )
				{
					byteAND++;
					maskAND = *byteAND;
					byteXOR++;
					maskXOR = *byteXOR;
				}
				else
				{
					maskAND <<= 1;
					maskXOR <<= 1;
				}
			}
		}
	}
	// copy image to texture#
	r = abs( bmColor.bmHeight ) * pitch;
	LONG paddBm = bmColor.bmWidthBytes - 4 * bmColor.bmWidth;
	LONG paddTx = pitch - 4 * bmColor.bmWidth;
	// swivel to argb
	for (BYTE *p = pData, *pE = pData + r, *pS = (BYTE*)bits; p != pE; p += paddBm, pS+= paddTx )
	{
		for (BYTE const* pLE = p + 4 * bmColor.bmWidth; p != pLE; p += 4, pS+= 4 )
		{
			pS[0] = p[3];
			pS[1] = p[0];
			pS[2] = p[1];
			pS[3] = p[2];
		}
	}
	free( pMask );
	free( pData );
	logStr( 3, "VERBOSE: Cursor image updated.\n" );
	return r;
}
#endif //def WIN32

////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_hModDll = hModule;
			//g_logLevel = 2;
			//strcpy_s( g_logFilePath, "VWB.log" );
			//logStr( 2, "bla" );
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			if (nullptr != ShowSystemCursor &&
				!g_bCurEnabled)
			{
				ShowSystemCursor(TRUE);
			}
			break;
	}
    return TRUE;
}
#elif defined __GNU__
  void __attribute__ ((constructor)) my_init(void)
  {
  }

  void __attribute__ ((destructor)) my_fini(void)
  {
  }
#endif /def WIN32
