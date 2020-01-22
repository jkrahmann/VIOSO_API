//#define SINEWAVE

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <sdkddkver.h>
#include <windows.h>
#else
#endif
#include "../../Include/EyePointProvider.h"
#include <stdint.h>
#include <math.h>
#include <sys/timeb.h>
#include "../../VIOSOWarpBlend/logging.h"
#include "../../VIOSOWarpBlend/socket.h"

HMODULE g_hModDll = 0;
bool bSineWave = false;
unsigned short port = 0;
EyePoint eyePointRcv;
uint64_t tmStamp = 0;
HANDLE hThread = 0;

#define M_PI       3.14159265358979323846   // pi
#define DEG2RADd( v ) ( (v) * double(M_PI / 180.0) )
#define RAD2DEGd( v ) ( (v) * double(180.0 / M_PI) )

struct Receiver
{
    uint64_t beginTime;
};
 

u_int __stdcall theadFn( void* param )
{
	logStr( 2, "start listener." );
	ifStartSockets()
	{
		Socket sock( SOCK_DGRAM, false, IPPROTO_UDP, false, 0, 0 );
		SocketAddress sa( INADDR_ANY, port );
		if( 0 != sock.bind( sa ) )
		{
			sock.close();
			return -1;
		}
		char buff[65535];
		while( sock )
		{
			int res = sock.recv( buff, 65535, INFINITETIMEOUT );
			if( SOCKET_ERROR == res )
			{
				return -2;
			}
			else if( 0 != res )
			{
				buff[res] = 0; // make zero terminated string
				if( 6 == sscanf_s( buff, "%lf %lf %lf %lf %lf %lf", &eyePointRcv.x, &eyePointRcv.y, &eyePointRcv.z, &eyePointRcv.pitch, &eyePointRcv.yaw, &eyePointRcv.roll ) )
				{
					eyePointRcv.x /= 1000;
					eyePointRcv.y /= 1000;
					eyePointRcv.z /= 1000;
					eyePointRcv.pitch *= DEG2RADd( 1 );
					eyePointRcv.yaw *= DEG2RADd( 1 );
					eyePointRcv.roll *= DEG2RADd( 1 );
					__timeb64 tm;
					_ftime_s( &tm );
					tmStamp = tm.time * 1000 + tm.millitm;
				}
			}
		}
	}
	return 0;
}

void* CreateEyePointReceiver( char const* szParam )
{
    auto r = new Receiver;
	__timeb64 tm; 
	_ftime_s( &tm );
	r->beginTime = tm.time * 1000 + tm.millitm;
	if( strstr( szParam, "sinewave" ) )
		bSineWave = true;
	else if( strstr( szParam, "listen" ) )
	{
		if( 1 == sscanf_s( szParam, "listen %hu", &port ) && 0 < port )
		{
			if( 0 == hThread )
				hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)theadFn, NULL, 0, NULL );
		}
	}
    return r;
}

// you need to fill the eyepoint with coordinates from the IG coordinate system
bool ReceiveEyePoint(void *receiver, EyePoint* eyePoint)
{
    auto r = static_cast<Receiver*>(receiver);
 
	__timeb64 tm; 
	_ftime_s( &tm );
	auto duration = tm.time * 1000 + tm.millitm;// - r->beginTime;
 
	const double tick = 5000.0;

	double ticks = double(duration) / tick;
 
    const double PI = 3.141592653589793e+00;

	const double mm[][12] =
	{
		{0,0,0,0,0,0,0,0,0,0,0,0},
		{1,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,1,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,1,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,1,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,1,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,1,0},
	};
 
	eyePoint->x = 0;
	eyePoint->y = 0;
    eyePoint->z = 0;
    eyePoint->yaw = 0; // around y
    eyePoint->roll = 0; // around x
    eyePoint->pitch = 0; // around z

	int d = sizeof(mm)/sizeof(mm[0]);
	ticks = fmod( ticks, d );

	int s = int(ticks);
	double o = fmod( ticks, 1 ) * 2 * PI;

	if( bSineWave )
	{
		eyePoint->x		= mm[s][ 1] + mm[s][ 0] * sin( o );
		eyePoint->y		= mm[s][ 3] + mm[s][ 2] * sin( o );
		eyePoint->z		= mm[s][ 5] + mm[s][ 4] * sin( o );
		eyePoint->yaw	= mm[s][ 7] + mm[s][ 6] * sin( o );
		eyePoint->pitch	= mm[s][ 9] + mm[s][ 8] * sin( o );
		eyePoint->roll  = mm[s][11] + mm[s][10] * sin( o );
	}
	else if( 0 != port )
	{
		*eyePoint = eyePointRcv;
	}
	/*
	if( bSineWave )
	{
		switch( int(secs/4) )
		{
		case 0: // moving center - right - center - left - center
			eyePoint->x = sin( PI * secs/8 );
			break;
		case 1: // moving center - right - center - left - center
			eyePoint->x = 1;
			break;
		case 2: // moving center - right - center - left - center
			eyePoint->x = sin( PI * ( secs/8 - 0.5 ) );
			break;
		case 3: // turning center - right - center - left - center (30deg)
			eyePoint->yaw = sin( PI * ( secs/8 - 1.5 ) ) / 6 * PI;
			break;
		case 4: // turning center - right - center - left - center (30deg)
			eyePoint->yaw = PI / 6;
			break;
		case 5: // turning center - right - center - left - center (30deg)
			eyePoint->yaw = sin( PI * ( secs/8 - 2 ) ) / 6 * PI;
			break;
		case 6: // moving center - up - center - down - center
			eyePoint->y = sin( PI * ( secs/8 - 3 ) );
			break;
		case 7: // moving center - up - center - down - center
			eyePoint->y = 1;
			break;
		case 8: // moving center - up - center - down - center
			eyePoint->y = sin( PI * ( secs/8 - 3.5 ) );
			break;
		case 9: // turning center - up - center - down - center (30deg)
			eyePoint->pitch = sin( PI * ( secs/8 - 4.5 ) ) / 6 * PI;
			break;
		case 10: // turning center - up - center - down - center (30deg)
			eyePoint->pitch = PI / 6;
			break;
		case 11: // turning center - up - center - down - center (30deg)
			eyePoint->pitch = sin( PI * ( secs/8 - 5 ) ) / 6 * PI;
			break;
		case 12: // moving center - up - center - down - center
			eyePoint->z = sin( PI * ( secs/8 - 6 ) );
			break;
		case 13: // moving center - up - center - down - center
			eyePoint->z = 1;
			break;
		case 14: // moving center - up - center - down - center
			eyePoint->z = sin( PI * ( secs/8 - 6.5 ) );
			break;
		case 15: // turning center - up - center - down - center (30deg)
			eyePoint->roll = sin( PI * ( secs/8 - 7.5 ) ) / 6 * PI;
			break;
		case 16: // turning center - up - center - down - center (30deg)
			eyePoint->roll = PI / 6;
			break;
		case 17: // turning center - up - center - down - center (30deg)
			eyePoint->roll = sin( PI * ( secs/8 - 8 ) ) / 6 * PI;
			break;
		case 19: // moving center - right - center - left - center
			eyePoint->x = sin( PI * ( secs/8 - 9.5 ) );
			break;
		case 20: // moving center - right - center - left - center
			eyePoint->x = 1;
			eyePoint->yaw = sin( PI * ( secs/8 - 10 ) ) / 6 * PI;
			break;
		case 21: // moving center - right - center - left - center
			eyePoint->x = 1;
			eyePoint->yaw = PI / 6;
			break;
		case 22: // turning center - right - center - left - center (30deg)
			eyePoint->x = 1;
			eyePoint->yaw = sin( PI * ( secs/8 - 10.5 ) ) / 6 * PI;
			break;
		case 23: // turning center - right - center - left - center (30deg)
			eyePoint->x = sin( PI * ( secs/8 - 11 ) );
			break;
		}
	}
	*/
    return true;
}
 
void DeleteEyePointReceiver(void* handle)
{
    delete static_cast<Receiver*>(handle);
}
 

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			g_hModDll = hModule;
			strcpy_s( g_logFilePath, "VIOSOWarpBlend.log" );
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
    return TRUE;
}

