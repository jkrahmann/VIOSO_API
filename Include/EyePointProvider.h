#ifndef VWB_EYEPOINTPROVIDER_HPP
#define VWB_EYEPOINTPROVIDER_HPP

typedef struct EyePoint
{
    double x, y, z;
    double roll, pitch, yaw;
} EyePoint;

#endif //ndef VWB_EYEPOINTPROVIDER_HPP

#if defined( EYEPOINTPROVIDER_EXPORTS )
#if defined(_MSC_VER)
#define EYEPOINTPROVIDER_DEF( ret, name, args )\
extern "C" __declspec(dllexport) ret name args;
#elif defined(__GNUC__)
#define EYEPOINTPROVIDER_DEF( ret, name, args )\
extern "C" __attribute__((visibility("default"))) ret name args;
#endif
#else
#define EYEPOINTPROVIDER_DEF( ret, name, args )\
	typedef ret (*pfn_##name)args;\
	extern pfn_##name name;
#endif

EYEPOINTPROVIDER_DEF( void*, CreateEyePointReceiver, ( char const* szParam ))
EYEPOINTPROVIDER_DEF( bool, ReceiveEyePoint, (void *receiver, EyePoint* eyePoint))
EYEPOINTPROVIDER_DEF( void, DeleteEyePointReceiver, (void* handle))

#undef EYEPOINTPROVIDER_DEF