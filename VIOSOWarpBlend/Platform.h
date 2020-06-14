#ifndef VWB_PLATFORM_INCLUDE_HPP
#define VWB_PLATFORM_INCLUDE_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#define _CRT_SECURE_NO_WARNINGS
#define __Q(x) #x
#define _Q(x) __Q(x)

#ifndef WIN32 // or something like that...
#define __stdcall
#endif

#if defined( _MSC_VER )
#if _MSC_VER <= 1600
#include <numeric>
#define NAN std::numeric_limits<float>::quiet_NaN()
#define VWB_inet_ntop_cast PVOID
#else
#define VWB_inet_ntop_cast IN_ADDR*
#endif //_MSC_VER <= 1600
#define _inline_ __forceinline
#else
#define _inline_ inline
#ifdef __linux__
#define VWB_inet_ntop_cast in_addr*
#else
#define VWB_inet_ntop_cast IN_ADDR*
#endif
#endif //defined( _MSC_VER )

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#define NOMINMAX
#include <sdkddkver.h>
#include <windows.h>
#include <stdint.h>
extern HMODULE g_hModDll;

#define sleep( x ) Sleep(x)

#elif __APPLE__
#include "mac_compat.h"
#else
#include "linux_compat.h"
#endif

#include <thread>
#include <mutex>

class VWB_MutexLock
{
private:
	std::mutex* m_h;
public:
	VWB_MutexLock( std::mutex& h, bool bTry = false ) : m_h( &h )
	{
		if( bTry )
		{
			if( !m_h->try_lock() )
			{
				m_h = nullptr;
			}
		}
		else
			m_h->lock();
	}
	~VWB_MutexLock()
	{
		if( nullptr != m_h )
			m_h->unlock();
	}
	operator bool() { return nullptr != m_h; }
};
#define VWB_LockedStatement( mx ) if( VWB_MutexLock __l = VWB_MutexLock( mx, false ) )
#define VWB_TryLockStatement( mx ) if( VWB_MutexLock __l = VWB_MutexLock( mx, true ) )
//#define VWB_LockedStatement( mx ) if( VWB_MutexLock __ml(mx) )
//#define VWB_TryLockStatement( mx, t_ms ) if( VWB_MutexLock _ml(mx,true) )


#endif //ndef VWB_PLATFORM_INCLUDE_HPP


