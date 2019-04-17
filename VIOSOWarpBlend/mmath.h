#ifndef DEF_MMATH_HPP
#define DEF_MMATH_HPP

#include "Platform.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include "../Include/VWBTypes.h"

#define DEG2RAD( v ) ( (v) * VWB_float(M_PI / 180) )
#define RAD2DEG( v ) ( (v) * VWB_float(180 / M_PI) )
#define DEG2RADf( v ) ( (v) * VWB_float(M_PI / 180) )
#define RAD2DEGf( v ) ( (v) * VWB_float(180 / M_PI) )
#define DEG2RADd( v ) ( (v) * VWB_double(M_PI / 180) )
#define RAD2DEGd( v ) ( (v) * VWB_double(180 / M_PI) )

template<class T>
struct VWB_MATRIX;

template<class T>
struct VWB_MATRIX33;

template<class T>
struct VWB_VECTOR4;

#pragma pack( push, 4 )
template<class T>
struct VWB_VECTOR3
{
	T x;
	T y;
	T z;
	_inline_ VWB_VECTOR3() {};
	_inline_ VWB_VECTOR3( VWB_VECTOR3 const& other ) { memcpy( this, &other, sizeof( *this ) ); }
    // without the following initualizer commented out gcc complains about amiguity (with the preceding initialzer?)
	//_inline_ VWB_VECTOR3( T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_VECTOR3( T _x, T _y, T _z ) : x(_x), y(_y), z(_z) {}

	_inline_ static VWB_VECTOR3 const& ptr( T const* p ) { return *(VWB_VECTOR3 const*)p; }
	_inline_ static VWB_VECTOR3& ptr( T* p ) { return *(VWB_VECTOR3*)p; }
	_inline_ static VWB_VECTOR3 O() { return VWB_VECTOR3(0,0,0); }
	_inline_ static VWB_VECTOR3 Ex() { return VWB_VECTOR3(1,0,0); }
	_inline_ static VWB_VECTOR3 Ey() { return VWB_VECTOR3(0,1,0); }
	_inline_ static VWB_VECTOR3 Ez() { return VWB_VECTOR3(0,0,1); }
	// gets barycentric coordinates from p placed on a triangle p1-p2-p3, z-coordinate must be the same on all vectors!
	_inline_ static bool Cart2Bary( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& p, VWB_VECTOR3& l ) 
	{

		VWB_VECTOR3 v0 = p2 - p1, v1 = p3 - p1, v2 = p - p1;
		T d00 = v0.dot(v0);
		T d01 = v0.dot(v1);
		T d11 = v1.dot(v1);
		T d20 = v2.dot(v0);
		T d21 = v2.dot(v1);
		T denom = T(1) / ( d00 * d11 - d01 * d01 );
		l.x = (d11 * d20 - d01 * d21) * denom;
		l.y = (d00 * d21 - d01 * d20) * denom;
		l.z = T(1) - l.x - l.y;
		if( 0 <= l.x && l.x <= 1 && 0 <= l.y && l.y <= 1 && 0 <= l.z && l.z <= 1 )
			return true;
		else
			return false;
	}

	_inline_ static bool Bary2Cart( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& l, VWB_VECTOR3& p ) 
	{
		p.x = l.x * p1.x + l.y * p2.x + l.z * p3.x;
		p.y = l.x * p1.y + l.y * p2.y + l.z * p3.y;
		p.z = l.x * p1.z + l.y * p2.z + l.z * p3.z;
		return true;
	}

	_inline_ operator T const* () const { return &x; }
	_inline_ operator T* () { return &x; }
	template< class _T2 >
	_inline_ VWB_VECTOR3& operator=( VWB_VECTOR3<_T2> const& other ) {
		x = (T)other.x; y = (T)other.y; z = (T)other.z;
		return *this; }
	_inline_ T lenSq() const { return x * x + y * y + z * z; }
	_inline_ T len() const { return sqrt( lenSq() ); }

	_inline_ VWB_VECTOR3 operator-() const { return VWB_VECTOR3( -x, -y, -z ); }
	_inline_ bool operator==(VWB_VECTOR3 const& other) const { return x == other.x && y == other.y && z == other.z; }
	_inline_ bool operator!=(VWB_VECTOR3 const& other) const { return x != other.x || y != other.y || z != other.z; }

	// multiply and divide
	_inline_ T dot( VWB_VECTOR3 const& other ) const// dot
	{
		T res = x * other.x + y * other.y + z * other.z;
		return res;
	}
	_inline_ VWB_VECTOR3 operator*( T const& other ) const// scalar
	{
		VWB_VECTOR3 res( x * other, y * other, z * other );
		return res;
	}
	_inline_ VWB_VECTOR3 operator/( T const& other ) const// scalar
	{
		VWB_VECTOR3 res( x / other, y / other, z / other );
		return res;
	}
	_inline_ VWB_VECTOR3 operator*( VWB_VECTOR3 const& other ) const// cross
	{
		return VWB_VECTOR3( y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x );
	}
	_inline_ VWB_VECTOR3& operator*=( T const other )
	{
		x*= other; 
		y*= other; 
		z*= other; 
		return *this;
	}
	_inline_ VWB_VECTOR3& operator/=( T const other )
	{
		x/= other; 
		y/= other; 
		z/= other; 
		return *this;
	}
	// add and subtract
	_inline_ VWB_VECTOR3 operator+( VWB_VECTOR3 const& other ) const
	{
		VWB_VECTOR3 res( x + other.x, y + other.y, z + other.z );
		return res;
	}
	_inline_ VWB_VECTOR3 operator-( VWB_VECTOR3 const& other ) const
	{
		VWB_VECTOR3 res( x - other.x, y - other.y, z - other.z );
		return res;
	}
	_inline_ VWB_VECTOR3& operator+=( VWB_VECTOR3 const& other )
	{
		x+= other.x; 
		y+= other.y; 
		z+= other.z; 
		return *this;
	}
	_inline_ VWB_VECTOR3& operator-=( VWB_VECTOR3 const& other )
	{
		x-= other.x; 
		y-= other.y; 
		z-= other.z; 
		return *this;
	}
	// with other types
	_inline_ VWB_VECTOR3& operator*=( VWB_MATRIX33<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31;
		T ya = x * M._12 + y * M._22 + z * M._32;
		z = x * M._13 + y * M._23 + z * M._33;
		x = xa;
		y = ya;
		return *this;
	}
	_inline_ VWB_VECTOR3& operator*=( VWB_MATRIX<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31 + M._41;
		T ya = x * M._12 + y * M._22 + z * M._32 + M._42;
		T w  = x * M._14 + y * M._24 + z * M._34 + M._44;
		z    = x * M._13 + y * M._23 + z * M._33 + M._43;
		x = xa / w;
		y = ya / w;
		z/= w;	
		return *this;
	}
	_inline_ VWB_VECTOR3 operator*( VWB_MATRIX33<T> const& M ) const
	{
		return VWB_VECTOR3( 
			x * M._11 + y * M._21 + z * M._31,
			x * M._12 + y * M._22 + z * M._32,
			x * M._13 + y * M._23 + z * M._33 );
	}
	_inline_ VWB_VECTOR3 operator*( VWB_MATRIX<T> const& M ) const
	{
		T w  = x * M._14 + y * M._24 + z * M._34 + M._44;
		return VWB_VECTOR3( 
			( x * M._11 + y * M._21 + z * M._31 + M._41 ) / w,
			( x * M._12 + y * M._22 + z * M._32 + M._42 ) / w,
			( x * M._13 + y * M._23 + z * M._33 + M._43 ) / w );
	}


	_inline_ void SetPtr( T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}
	_inline_ VWB_VECTOR3& normalize()
	{
		T l = len();
		if( FLT_MIN < l || -FLT_MIN > l )
		{
			x/= l;
			y/= l;
			z/= l;
		}
		return *this;
	}
	_inline_ VWB_VECTOR3 normalized() const
	{
		VWB_VECTOR3 r;
		T l = len();
		r.x = x / l;
		r.y = y / l;
		r.z = z / l;
		return r;
	}

};

template< class T >
struct VWB_VECTOR4
{
	T x;
	T y;
	T z;
	T w;
	_inline_ VWB_VECTOR4() {};
	_inline_ VWB_VECTOR4( VWB_VECTOR4 const& other ) { memcpy( this, &other, sizeof( *this ) ); }
	_inline_ VWB_VECTOR4( VWB_VECTOR3<T> const& other ) { memcpy( this, &other, sizeof( other ) ); w = 1; }
	_inline_ VWB_VECTOR4( T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_VECTOR4( T _x, T _y, T _z, T _w ) : x(_x), y(_y), z(_z), w(_w) {}
	_inline_ static VWB_VECTOR4 const& ptr( T const* p ) { return *(VWB_VECTOR4 const*)p; }
	_inline_ static VWB_VECTOR4& ptr( T* p ) { return *(VWB_VECTOR4*)p; }
	_inline_ static VWB_VECTOR4 O() { return VWB_VECTOR4(0,0,0,0); }
	_inline_ static VWB_VECTOR4 O1() { return VWB_VECTOR4(0,0,0,1); }
	_inline_ static VWB_VECTOR4 Ex() { return VWB_VECTOR4(1,0,0,1); }
	_inline_ static VWB_VECTOR4 Ey() { return VWB_VECTOR4(0,1,0,1); }
	_inline_ static VWB_VECTOR4 Ez() { return VWB_VECTOR4(0,0,1,1); }
	_inline_ operator VWB_VECTOR3<T>& () { return *(VWB_VECTOR3<T>*)this; }
	_inline_ operator VWB_VECTOR3<T> const& () const { return *(VWB_VECTOR3<T>*)this; }
	_inline_ operator T const* () const { return &x; }
	_inline_ operator T* () { return &x; }
	template< class _T2 >
	_inline_ VWB_VECTOR4& operator=( VWB_VECTOR4<_T2> const& other ) {
		x = (T)other.x; y = (T)other.y; z = (T)other.z; w = (T)other.w;
		return *this; }

	_inline_ VWB_VECTOR4 operator-() const { return VWB_VECTOR4( -x, -y, -z, -w ); }
	_inline_ bool operator==(VWB_VECTOR4 const& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
	_inline_ bool operator!=(VWB_VECTOR4 const& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }

	// multiply and divide
	_inline_ T dot( VWB_VECTOR4 const& other ) const // dot
	{
		T res = x * other.x + y * other.y + z * other.z + w * other.w;
		return res;
	}
	_inline_ VWB_VECTOR4 operator*( T const& other ) const// scalar
	{
		VWB_VECTOR4 res( x * other, y * other, z * other, w * other );
		return res;
	}
	_inline_ VWB_VECTOR4 operator/( T const& other ) const// scalar
	{
		VWB_VECTOR4 res( x / other, y / other, z / other, w / other );
		return res;
	}
	_inline_ VWB_VECTOR4& operator*=( T const other )
	{
		x*= other; 
		y*= other; 
		z*= other; 
		w*= other;
		return *this;
	}
	_inline_ VWB_VECTOR4& operator/=( T const other )
	{
		x/= other; 
		y/= other; 
		z/= other; 
		w/= other;
		return *this;
	}

	// add and subtract
	_inline_ VWB_VECTOR4 operator+( VWB_VECTOR4 const& other ) const
	{
		VWB_VECTOR4 res( x + other.x, y + other.y, z + other.z, w + other.w );
		return res;
	}
	_inline_ VWB_VECTOR4 operator-( VWB_VECTOR4 const& other ) const
	{
		VWB_VECTOR4 res( x - other.x, y - other.y, z - other.z, w - other.w );
		return res;
	}
	_inline_ VWB_VECTOR4& operator+=( VWB_VECTOR4 const& other )
	{
		x+= other.x; 
		y+= other.y; 
		z+= other.z; 
		w+= other.w; 
		return *this;
	}
	_inline_ VWB_VECTOR4& operator-=( VWB_VECTOR4 const& other )
	{
		x-= other.x; 
		y-= other.y; 
		z-= other.z; 
		w-= other.w; 
		return *this;
	}

	_inline_ VWB_VECTOR4& Cons()
	{
		x/= w; 
		y/= w; 
		z/= w; 
		w = 1; 
		return *this;
	}

	_inline_ VWB_VECTOR4& operator*=( VWB_MATRIX<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31 + w * M._41;
		T ya = x * M._12 + y * M._22 + z * M._32 + w * M._42;
		T za = x * M._13 + y * M._23 + z * M._33 + w * M._43;
		w  =   x * M._14 + y * M._24 + z * M._34 + w * M._44;
		x = xa;
		y = ya;
		z = za;	
		return *this;
	}
	_inline_ VWB_VECTOR4 operator* ( VWB_MATRIX<T> const& M ) const
	{
		return VWB_VECTOR4( 
			x * M._11 + y * M._21 + z * M._31 + w * M._41,
			x * M._12 + y * M._22 + z * M._32 + w * M._42,
			x * M._13 + y * M._23 + z * M._33 + w * M._43,
			x * M._14 + y * M._24 + z * M._34 + w * M._44 );
	}
	_inline_ void SetPtr( T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}
};

template< class _T >
struct VWB_MATRIX
{
    union {
        struct {
            _T        _11, _12, _13, _14;
            _T        _21, _22, _23, _24;
            _T        _31, _32, _33, _34;
            _T        _41, _42, _43, _44;

        };
        _T m[4][4];
        _T p[16];
    };
	_inline_ VWB_MATRIX() {};
	_inline_ VWB_MATRIX( VWB_MATRIX const& other ) { memcpy( this, &other, sizeof( *this ) ); }
	_inline_ VWB_MATRIX( VWB_MATRIX33<_T> const& other ) : 
		_11( other._11 ), _12( other._12 ), _13( other._13 ), _14(0),
		_21( other._21 ), _22( other._22 ), _23( other._23 ), _24(0), 
		_31( other._31 ), _32( other._32 ), _33( other._33 ), _34(0),
		_41(0), _42(0), _43(0), _44(1) {}
	template<class _T2>
	_inline_ VWB_MATRIX( VWB_MATRIX<_T2> const& other ) : 
		_11( (_T)other._11 ), _12( (_T)other._12 ), _13( (_T)other._13 ), _14( (_T)other._14 ),
		_21( (_T)other._21 ), _22( (_T)other._22 ), _23( (_T)other._23 ), _24( (_T)other._24 ), 
		_31( (_T)other._31 ), _32( (_T)other._32 ), _33( (_T)other._33 ), _34( (_T)other._34 ),
		_41( (_T)other._41 ), _42( (_T)other._42 ), _43( (_T)other._43 ), _44( (_T)other._44 ){}
	_inline_ VWB_MATRIX( _T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_MATRIX( _T f11, _T f12, _T f13, _T f14,
			  _T f21, _T f22, _T f23, _T f24,
			  _T f31, _T f32, _T f33, _T f34,
			  _T f41, _T f42, _T f43, _T f44 ) :
		_11(f11), _12(f12), _13(f13), _14(f14),
		_21(f21), _22(f22), _23(f23), _24(f24),
		_31(f31), _32(f32), _33(f33), _34(f34),
		_41(f41), _42(f42), _43(f43), _44(f44) {}
	template< class _T2 >
	_inline_ VWB_MATRIX& operator=( VWB_MATRIX<_T2> const& other ) {
		_11 = (_T)other._11; _12 = (_T)other._12; _13 = (_T)other._13; _14 = (_T)other._14;
		_21 = (_T)other._21; _22 = (_T)other._22; _23 = (_T)other._23; _24 = (_T)other._24; 
		_31 = (_T)other._31; _32 = (_T)other._32; _33 = (_T)other._33; _34 = (_T)other._34;
		_41 = (_T)other._41; _42 = (_T)other._42; _43 = (_T)other._43; _44 = (_T)other._44; 
		return *this; }
	_inline_ static VWB_MATRIX & ptr( _T* p ) { return *(VWB_MATRIX*)p; }
	_inline_ static VWB_MATRIX const& ptr( _T const* p ) { return *(VWB_MATRIX const*)p; }
	_inline_ static VWB_MATRIX I() { return VWB_MATRIX( 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);}
	_inline_ static VWB_MATRIX O() { VWB_MATRIX res; memset( res, 0, sizeof( res ) ); return res;}
	_inline_ static VWB_MATRIX S( VWB_VECTOR4<_T> const& scale) { return VWB_MATRIX( scale.x, 0, 0, 0,  0, scale.y, 0, 0,  0, 0, scale.z, 0,  0, 0, 0, scale.w );}
	_inline_ static VWB_MATRIX S( _T const& sx, _T const& sy, _T const& sz, _T const& sw ) { return VWB_MATRIX( sx, 0, 0, 0,  0, sy, 0, 0,  0, 0, sz, 0,  0, 0, 0, sw );}
	_inline_ static VWB_MATRIX FlipHanddedness() { return VWB_MATRIX( 1,0,0,0,  0,1,0,0, 0,0,-1,0, 0,0,0,1 ); }

	_inline_ static VWB_MATRIX T( VWB_VECTOR3<_T> const& t ) { VWB_MATRIX res( 1,0,0,t.x,0,1,0,t.y,0,0,1,t.z,0,0,0,1); return res;}
	_inline_ static VWB_MATRIX T( _T x, _T y, _T z ) { VWB_MATRIX res( 1,0,0,x,0,1,0,y,0,0,1,z,0,0,0,1); return res;}

	_inline_ static VWB_MATRIX Rx( _T x ) { return VWB_MATRIX( 1,0,0,0,  0,cos(x),-sin(x),0,  0,sin(x),cos(x),0, 0,0,0,1 ); }
	_inline_ static VWB_MATRIX Ry( _T y ) { return VWB_MATRIX( cos(y),0,-sin(y),0,  0,1,0,0, sin(y),0,cos(y),0, 0,0,0,1 ); } 
	_inline_ static VWB_MATRIX Rz( _T z ) { return VWB_MATRIX( cos(z),sin(z),0,0,  -sin(z),cos(z),0,0, 0,0,1,0, 0,0,0,1 ); }
	_inline_ static VWB_MATRIX R( _T x, _T y, _T z )  // this is Rz(z) * Rx(x) * Ry(y)
	{ 
		return VWB_MATRIX(
			 cos(y)*cos(z)-sin(x)*sin(y)*sin(z),	cos(x)*sin(z),	-sin(x)*cos(y)*sin(z)-sin(y)*cos(z),0,
			-sin(x)*sin(y)*cos(z)-cos(y)*sin(z),	 cos(x)*cos(z),	 sin(y)*sin(z)-sin(x)*cos(y)*cos(z),0,
			 cos(x)*sin(y),							 sin(x),		cos(x)*cos(y),0, 0,0,0,1 );
	}
	_inline_ static VWB_MATRIX R( VWB_VECTOR3<_T> r )  // this is Rz(z) * Rx(x) * Ry(y)
	{ 
		return VWB_MATRIX33<_T>(
			 cos(r.y)*cos(r.z)-sin(r.x)*sin(r.y)*sin(r.z),	cos(r.x)*sin(r.z),	-sin(r.x)*cos(r.y)*sin(r.z)-sin(r.y)*cos(r.z),0,
			-sin(r.x)*sin(r.y)*cos(r.z)-cos(r.y)*sin(r.z),	 cos(r.x)*cos(r.z),	sin(r.y)*sin(r.z)-sin(r.x)*cos(r.y)*cos(r.z),0,
			 cos(r.x)*sin(r.y),							 sin(r.x),		cos(r.x)*cos(r.y),0, 0,0,0,1 );
	}

	// transposed ones
	_inline_ static VWB_MATRIX TT( VWB_VECTOR3<_T> const& t ) { VWB_MATRIX res( 1,0,0,0,0,1,0,0,0,0,1,0,t.x,t.y,t.z,1); return res;}
	_inline_ static VWB_MATRIX TT( _T x, _T y, _T z ) { VWB_MATRIX res( 1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1); return res;}
	// transposed matrices
	_inline_ static VWB_MATRIX RxLHT( _T x ) { return VWB_MATRIX( 1,0,0,0,  0,cos(x),sin(x),0,  0,-sin(x),cos(x),0, 0,0,0,1 ); }
	_inline_ static VWB_MATRIX RyLHT( _T y ) { return VWB_MATRIX(cos(y),0,-sin(y),0,  0,1,0,0, sin(y),0,cos(y),0,  0,0,0,1 ); } 
	_inline_ static VWB_MATRIX RzLHT( _T z ) { return VWB_MATRIX(cos(z),sin(z),0,0,  -sin(z),cos(z),0,0, 0,0,1,0,  0,0,0,1 ); }
	_inline_ static VWB_MATRIX RLHT( _T x, _T y, _T z ) // this is RyT(y) * RxT(x) * RzT(z)
	{ 
		return VWB_MATRIX( 
			 cos(y)*cos(z)-sin(x)*sin(y)*sin(z),	-sin(x)*sin(y)*cos(z)-cos(y)*sin(z),	-cos(x)*sin(y), 0,
			 cos(x)*sin(z),							cos(x)*cos(z),						 -sin(x), 0,
			 sin(x)*cos(y)*sin(z)+sin(y)*cos(z),	-sin(y)*sin(z)+sin(x)*cos(y)*cos(z),	 cos(x)*cos(y), 0, 
			 0,0,0,1 );
	}
	_inline_ static VWB_MATRIX RLHT( VWB_VECTOR3<_T> r) // this is RyT(y) * RxT(x) * RzT(z)
	{ 
		return VWB_MATRIX( 
			 cos(r.y)*cos(r.z)-sin(r.x)*sin(r.y)*sin(r.z),	-sin(r.x)*sin(r.y)*cos(r.z)-cos(r.y)*sin(r.z),	-cos(r.x)*sin(r.y),0,
			 cos(r.x)*sin(r.z),							cos(r.x)*cos(r.z),						 -sin(r.x),0,
			 sin(r.x)*cos(r.y)*sin(r.z)+sin(r.y)*cos(r.z),	-sin(r.y)*sin(r.z)+sin(r.x)*cos(r.y)*cos(r.z),	 cos(r.x)*cos(r.y),0,
			 0,0,0,1 );
	}

	_inline_ static VWB_MATRIX PLHT( VWB_VECTOR4<_T> const& viewsizes, _T nearDist, _T farDist, _T screenDist, VWB_VECTOR3<_T> const& eye )
	{
		//2*zn/(r-l)   0            0              0
		//0            2*zn/(t-b)   0              0
		//(l+r)/(l-r)  (t+b)/(b-t)  zf/(zf-zn)     1
		//0            0            zn*zf/(zn-zf)  0

		//float d = m_near / ( m_dist - e.z ); // nearDist = (screenDist - eye[2]) * d
		//	d * ( -m_viewSizes.x - e.x ), //l
		//	d * ( m_viewSizes.z - e.x ), //r

		//	d * ( -m_viewSizes.y - e.y ), //b
		//	d * ( m_viewSizes.w - e.y ), //t

		//_22 = 2 * (screenDist - eye[2]) / ( ( m_viewSizes.w - e.y ) - ( -m_viewSizes.y - e.y ))
		return VWB_MATRIX(
			(_T)2 * ( screenDist - eye[2] ) / ( viewsizes[2] + viewsizes[0] ),
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			(_T)2 * ( screenDist - eye[2] ) / ( viewsizes[3] + viewsizes[1] ),
			(_T)0,
			(_T)0,

			(_T)( viewsizes[0] - viewsizes[2] + 2 * eye[0] ) / ( viewsizes[0] + viewsizes[2] ),
			(_T)( viewsizes[1] - viewsizes[3] + 2 * eye[1] ) / ( viewsizes[1] + viewsizes[3] ),
			( farDist + nearDist ) / ( farDist - nearDist ),
			(_T)1,

			(_T)0,
			(_T)0,
			(_T)nearDist * farDist / ( nearDist - farDist ),
			(_T)0 );
	}

	_inline_ static VWB_MATRIX PRHT( VWB_VECTOR4<_T> const& viewsizes, _T nearDist, _T farDist, _T screenDist, VWB_VECTOR3<_T> eye )
	{
		return VWB_MATRIX(
			(_T)2 * ( screenDist + eye[2] ) / ( viewsizes[2] + viewsizes[0] ),
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			(_T)2 * ( screenDist + eye[2] ) / ( viewsizes[3] + viewsizes[1] ),
			(_T)0,
			(_T)0,

			(_T)( viewsizes[2] - viewsizes[0] - 2 * eye[0] ) / ( viewsizes[0] + viewsizes[2] ),
			(_T)( viewsizes[3] - viewsizes[1] - 2 * eye[1] ) / ( viewsizes[1] + viewsizes[3] ),
			2 * ( farDist + nearDist ) / ( nearDist - farDist ),
			(_T)-1,

			(_T)0,
			(_T)0,
			(_T)nearDist * farDist / ( nearDist - farDist ),
			(_T)0 );
	}

	_inline_ static VWB_MATRIX PRH( VWB_VECTOR4<_T> viewsizes, _T nearDist, _T farDist, _T screenDist, VWB_VECTOR3<_T> eye )
	{
		return VWB_MATRIX(
			/* 0*/ (_T)2 * ( screenDist + eye[2] ) / ( viewsizes[2] + viewsizes[0] ),
			/* 1*/ (_T)0,
			/* 2*/ (_T)0,
			/* 3*/ (_T)0,

			/* 4*/ (_T)0,
			/* 5*/ (_T)2 * ( screenDist + eye[2] ) / ( viewsizes[3] + viewsizes[1] ),
			/* 6*/ (_T)0,
			/* 7*/ (_T)0,

			/* 8*/ (_T)( viewsizes[0] - viewsizes[2] - 2 * eye[0] ) / ( viewsizes[0] + viewsizes[2] ),
			/* 9*/ (_T)( viewsizes[1] - viewsizes[3] - 2 * eye[1] ) / ( viewsizes[1] + viewsizes[3] ),
			/*10*/ (_T)( farDist + nearDist ) / ( nearDist - farDist ),
			/*11*/ (_T)-1,

			/*12*/ (_T)0,
			/*13*/ (_T)0,
			/*14*/ (_T)2 * nearDist * farDist / ( nearDist - farDist ),
			/*15*/ (_T)0 );
	}
	_inline_ static VWB_MATRIX PLH( VWB_VECTOR4<_T> viewsizes, _T nearDist, _T farDist, _T screenDist, VWB_VECTOR3<_T> eye )
	{
		return VWB_MATRIX(
			/* 0*/ (_T)2 * ( screenDist - eye[2] ) / ( viewsizes[2] + viewsizes[0] ),
			/* 1*/ (_T)0,
			/* 2*/ (_T)0,
			/* 3*/ (_T)0,

			/* 4*/ (_T)0,
			/* 5*/ (_T)2 * ( screenDist - eye[2] ) / ( viewsizes[3] + viewsizes[1] ),
			/* 6*/ (_T)0,
			/* 7*/ (_T)0,

			/* 8*/ (_T)( viewsizes[2] - viewsizes[0] - 2 * eye[0] ) / ( viewsizes[0] + viewsizes[2] ),
			/* 9*/ (_T)( viewsizes[3] - viewsizes[1] - 2 * eye[1] ) / ( viewsizes[1] + viewsizes[3] ),
			/*10*/ (_T)( farDist + nearDist ) / ( farDist - nearDist ),
			/*11*/ (_T)1,

			/*12*/ (_T)0,
			/*13*/ (_T)0,
			/*14*/ (_T)2 * nearDist * farDist / ( nearDist - farDist ),
			/*15*/ (_T)0 );
	}



	_inline_ static VWB_MATRIX DXFrustumLH( _T const* pClip )
	{
		return VWB_MATRIX(
			(_T)2 * pClip[4] / ( pClip[2] + pClip[0] ),
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			(_T)2 * pClip[4] / ( pClip[3] + pClip[1] ),
			(_T)0,
			(_T)0,

			(_T)( pClip[0] - pClip[2] ) / ( pClip[0] + pClip[2] ),
			(_T)( pClip[1] - pClip[3] ) / ( pClip[1] + pClip[3] ),
			(_T)( pClip[5] + pClip[4] ) / ( pClip[5] - pClip[4] ),
			(_T)1,

			(_T)0,
			(_T)0,
			(_T)pClip[4] * pClip[5] / ( pClip[4] - pClip[5] ),
			(_T)0 );
	}

	_inline_ static VWB_MATRIX DXFrustumRH( _T const* pClip )
	{
		return VWB_MATRIX(
			(_T)2 * pClip[4] / ( pClip[2] + pClip[0] ),
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			(_T)2 * pClip[4] / ( pClip[3] + pClip[1] ),
			(_T)0,
			(_T)0,

			(_T)( pClip[2] - pClip[0] ) / ( pClip[0] + pClip[2] ),
			(_T)( pClip[3] - pClip[1] ) / ( pClip[1] + pClip[3] ),
			(_T)( pClip[5] + pClip[4] ) / ( pClip[4] - pClip[5] ),
			(_T)-1,

			(_T)0,
			(_T)0,
			(_T)pClip[4] * pClip[5] / ( pClip[5] - pClip[4] ),
			(_T)0 );
	}

	_inline_ static VWB_MATRIX GLFrustumRH( _T const* pClip )
	{
		return VWB_MATRIX(
			/* 0*/ (_T)2 * pClip[4] / ( pClip[2] + pClip[0] ),
			/* 1*/ (_T)0,
			/* 2*/ (_T)0,
			/* 3*/ (_T)0,

			/* 4*/ (_T)0,
			/* 5*/ (_T)2 * pClip[4] / ( pClip[3] + pClip[1] ),
			/* 6*/ (_T)0,
			/* 7*/ (_T)0,

			/* 8*/ (_T)( pClip[2] - pClip[0] ) / ( pClip[0] + pClip[2] ),
			/* 9*/ (_T)( pClip[3] - pClip[1] ) / ( pClip[1] + pClip[3] ),
			/*10*/ (_T)( pClip[5] + pClip[4] ) / ( pClip[4] - pClip[5] ),
			/*11*/ (_T)-1,

			/*12*/ (_T)0,
			/*13*/ (_T)0,
			/*14*/ (_T)2 * pClip[4] * pClip[5] / ( pClip[4] - pClip[5] ),
			/*15*/ (_T)0 );
	}
	_inline_ static VWB_MATRIX GLFrustumLH( _T const* pClip )
	{
		return VWB_MATRIX(
			/* 0*/ (_T)2 * pClip[4] / ( pClip[2] + pClip[0] ),
			/* 1*/ (_T)0,
			/* 2*/ (_T)0,
			/* 3*/ (_T)0,

			/* 4*/ (_T)0,
			/* 5*/ (_T)2 * pClip[4] / ( pClip[3] + pClip[1] ),
			/* 6*/ (_T)0,
			/* 7*/ (_T)0,

			/* 8*/ (_T)( pClip[0] - pClip[2] ) / ( pClip[0] + pClip[2] ),
			/* 9*/ (_T)( pClip[1] - pClip[3] ) / ( pClip[1] + pClip[3] ),
			/*10*/ (_T)( pClip[5] + pClip[4] ) / ( pClip[5] - pClip[4] ),
			/*11*/ (_T)1,

			/*12*/ (_T)0,
			/*13*/ (_T)0,
			/*14*/ (_T)2 * pClip[4] * pClip[5] / ( pClip[4] - pClip[5] ),
			/*15*/ (_T)0 );
	}


	_inline_ _T& operator()(unsigned int row, unsigned int col) { return m[row][col]; }
	_inline_ _T const& operator()(unsigned int row, unsigned int col) const { return m[row][col]; }
	_inline_ operator _T* () { return &_11; }
	_inline_ operator _T const* () const { return &_11; }
	//_inline_ _T* operator[]( int i ) { return m[i]; }
	//_inline_ _T* const* operator[]( int i ) const { return m[i]; }

	_inline_ VWB_MATRIX operator-() const { return VWB_MATRIX( -_11, -_12, -_13, -_14,
															   -_21, -_22, -_23, -_24,
															   -_31, -_32, -_33, -_34,
                                                               -_41, -_42, -_43, -_44 ); }
	_inline_ bool operator==(VWB_MATRIX const& other) const 
	{ 
		for( int i = 0; i != 16; i++ ) if( p[i] != other.p[i] ) return false;
		return true; 
	}
	_inline_ bool operator!=(VWB_MATRIX const& other) const 
	{ 
		for( int i = 0; i != 16; i++ ) if( p[i] != other.p[i] ) return true;
		return false; 
	}

	_inline_ VWB_MATRIX operator*( _T const other ) const
	{
		VWB_MATRIX res( _11 * other, _12 * other, _13 * other, _14 * other, 
					  _21 * other, _22 * other, _23 * other, _24 * other, 
					  _31 * other, _32 * other, _33 * other, _34 * other, 
					  _41 * other, _42 * other, _43 * other, _44 * other );
		return res;
	}
	_inline_ VWB_MATRIX& operator*=( _T const other )
	{
		_11*= other; _12*= other; _13*= other; _14*= other; 
		_21*= other; _22*= other; _23*= other; _24*= other; 
		_31*= other; _32*= other; _33*= other; _34*= other; 
		_41*= other; _42*= other; _43*= other; _44*= other;
		return *this;
	}
	_inline_ VWB_MATRIX& operator*=( VWB_MATRIX const& other )
	{
		VWB_MATRIX M = *this;
		*this = M * other;
		return *this;
	}
	_inline_ VWB_MATRIX operator*( VWB_MATRIX const& other ) const
	{
		VWB_MATRIX res;
		for( int i = 0; i != 4; i++ )
			for( int j = 0; j != 4; j++ )
			{
				res.m[i][j] = m[i][0] * other.m[0][j];
				for( int k = 1; k != 4; k++ )
					res.m[i][j]+= m[i][k] * other.m[k][j];
			}
		return res;
	}
	_inline_ VWB_VECTOR3<_T> operator*( VWB_VECTOR3<_T> const& other ) const
	{
		VWB_VECTOR3<_T> res;
		res.x = _11 * other.x + _12 * other.y + _13 * other.z + _14;
		res.y = _21 * other.x + _22 * other.y + _23 * other.z + _24;
		res.z = _31 * other.x + _32 * other.y + _33 * other.z + _34;
		_T w = _41 * other.x + _42 * other.y + _43 * other.z + _44;
		res.x/= w;
		res.y/= w;
		res.z/= w;
		return res;
	}
	_inline_ VWB_VECTOR4<_T> operator*( VWB_VECTOR4<_T> const& other ) const
	{
		VWB_VECTOR4<_T> res;
		res.x = _11 * other.x + _12 * other.y + _13 * other.z + _14 * other.w;
		res.y = _21 * other.x + _22 * other.y + _23 * other.z + _24 * other.w;
		res.z = _31 * other.x + _32 * other.y + _33 * other.z + _34 * other.w;
		res.w = _41 * other.x + _42 * other.y + _43 * other.z + _44 * other.w;
		return res;
	}

	_inline_ void SetPtr( _T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}

	_inline_ VWB_MATRIX& Transpose()
	{
		_T a = _12;
		_12 = _21;
		_21 = a;
		
		a = _13;
		_13 = _31;
		_31 = a;

		a = _14;
		_14 = _41;
		_41 = a;

		a = _23;
		_23 = _32;
		_32 = a;

		a = _24;
		_24 = _42;
		_42 = a;

		a = _34;
		_34 = _43;
		_43 = a;

		return *this;
	}

	_inline_ VWB_MATRIX Transposed()	{ return VWB_MATRIX ( _11, _21, _31, _41, _12, _22, _32, _42, _13, _23, _33, _43,_14, _24, _34, _44 ); }

	_inline_ VWB_MATRIX& Invert()
	{
		*this = Inverted();
		return *this;
	}
	_inline_ VWB_MATRIX Inverted() const
	{
		VWB_MATRIX inv;
		_T det;
		int i;

		inv[0] = p[5]  * p[10] * p[15] - 
				 p[5]  * p[11] * p[14] - 
				 p[9]  * p[6]  * p[15] + 
				 p[9]  * p[7]  * p[14] +
				 p[13] * p[6]  * p[11] - 
				 p[13] * p[7]  * p[10];

		inv[4] = -p[4]  * p[10] * p[15] + 
				  p[4]  * p[11] * p[14] + 
				  p[8]  * p[6]  * p[15] - 
				  p[8]  * p[7]  * p[14] - 
				  p[12] * p[6]  * p[11] + 
				  p[12] * p[7]  * p[10];

		inv[8] = p[4]  * p[9] * p[15] - 
				 p[4]  * p[11] * p[13] - 
				 p[8]  * p[5] * p[15] + 
				 p[8]  * p[7] * p[13] + 
				 p[12] * p[5] * p[11] - 
				 p[12] * p[7] * p[9];

		inv[12] = -p[4]  * p[9] * p[14] + 
				   p[4]  * p[10] * p[13] +
				   p[8]  * p[5] * p[14] - 
				   p[8]  * p[6] * p[13] - 
				   p[12] * p[5] * p[10] + 
				   p[12] * p[6] * p[9];

		inv[1] = -p[1]  * p[10] * p[15] + 
				  p[1]  * p[11] * p[14] + 
				  p[9]  * p[2] * p[15] - 
				  p[9]  * p[3] * p[14] - 
				  p[13] * p[2] * p[11] + 
				  p[13] * p[3] * p[10];

		inv[5] = p[0]  * p[10] * p[15] - 
				 p[0]  * p[11] * p[14] - 
				 p[8]  * p[2] * p[15] + 
				 p[8]  * p[3] * p[14] + 
				 p[12] * p[2] * p[11] - 
				 p[12] * p[3] * p[10];

		inv[9] = -p[0]  * p[9] * p[15] + 
				  p[0]  * p[11] * p[13] + 
				  p[8]  * p[1] * p[15] - 
				  p[8]  * p[3] * p[13] - 
				  p[12] * p[1] * p[11] + 
				  p[12] * p[3] * p[9];

		inv[13] = p[0]  * p[9] * p[14] - 
				  p[0]  * p[10] * p[13] - 
				  p[8]  * p[1] * p[14] + 
				  p[8]  * p[2] * p[13] + 
				  p[12] * p[1] * p[10] - 
				  p[12] * p[2] * p[9];

		inv[2] = p[1]  * p[6] * p[15] - 
				 p[1]  * p[7] * p[14] - 
				 p[5]  * p[2] * p[15] + 
				 p[5]  * p[3] * p[14] + 
				 p[13] * p[2] * p[7] - 
				 p[13] * p[3] * p[6];

		inv[6] = -p[0]  * p[6] * p[15] + 
				  p[0]  * p[7] * p[14] + 
				  p[4]  * p[2] * p[15] - 
				  p[4]  * p[3] * p[14] - 
				  p[12] * p[2] * p[7] + 
				  p[12] * p[3] * p[6];

		inv[10] = p[0]  * p[5] * p[15] - 
				  p[0]  * p[7] * p[13] - 
				  p[4]  * p[1] * p[15] + 
				  p[4]  * p[3] * p[13] + 
				  p[12] * p[1] * p[7] - 
				  p[12] * p[3] * p[5];

		inv[14] = -p[0]  * p[5] * p[14] + 
				   p[0]  * p[6] * p[13] + 
				   p[4]  * p[1] * p[14] - 
				   p[4]  * p[2] * p[13] - 
				   p[12] * p[1] * p[6] + 
				   p[12] * p[2] * p[5];

		inv[3] = -p[1] * p[6] * p[11] + 
				  p[1] * p[7] * p[10] + 
				  p[5] * p[2] * p[11] - 
				  p[5] * p[3] * p[10] - 
				  p[9] * p[2] * p[7] + 
				  p[9] * p[3] * p[6];

		inv[7] = p[0] * p[6] * p[11] - 
				 p[0] * p[7] * p[10] - 
				 p[4] * p[2] * p[11] + 
				 p[4] * p[3] * p[10] + 
				 p[8] * p[2] * p[7] - 
				 p[8] * p[3] * p[6];

		inv[11] = -p[0] * p[5] * p[11] + 
				   p[0] * p[7] * p[9] + 
				   p[4] * p[1] * p[11] - 
				   p[4] * p[3] * p[9] - 
				   p[8] * p[1] * p[7] + 
				   p[8] * p[3] * p[5];

		inv[15] = p[0] * p[5] * p[10] - 
				  p[0] * p[6] * p[9] - 
				  p[4] * p[1] * p[10] + 
				  p[4] * p[2] * p[9] + 
				  p[8] * p[1] * p[6] - 
				  p[8] * p[2] * p[5];

		det = p[0] * inv[0] + p[1] * inv[4] + p[2] * inv[8] + p[3] * inv[12];

		det = ((_T)1) / det;

		for (i = 0; i < 16; i++)
			inv[i]*= det;

		return inv;
	}

	_inline_ VWB_VECTOR4<_T>& X() { return *(VWB_VECTOR4<_T>*)&_11; }
	_inline_ VWB_VECTOR4<_T> const& X() const { return *(VWB_VECTOR4<_T>*)&_11; }
	_inline_ VWB_VECTOR4<_T>& Y() { return *(VWB_VECTOR4<_T>*)&_21; }
	_inline_ VWB_VECTOR4<_T> const& Y() const { return *(VWB_VECTOR4<_T>*)&_21; }
	_inline_ VWB_VECTOR4<_T>& Z() { return *(VWB_VECTOR4<_T>*)&_31; }
	_inline_ VWB_VECTOR4<_T> const& Z() const { return *(VWB_VECTOR4<_T>*)&_31; }
	_inline_ VWB_VECTOR4<_T>& W() { return *(VWB_VECTOR4<_T>*)&_41; }
	_inline_ VWB_VECTOR4<_T> const& W() const { return *(VWB_VECTOR4<_T>*)&_41; }
	
	_inline_ static VWB_MATRIX ORTHO( _T left, _T right, _T bottom, _T top, _T near, _T far)
	{
		_T ral = right + left;
		_T rsl = right - left;
		_T tab = top + bottom;
		_T tsb = top - bottom;
		_T fan = far + near;
		_T fsn = far - near;
		return VWB_MATRIX (
						   2 / rsl, 0, 0, -ral / rsl,
						   0, 2 / tsb, 0, -tab / tsb,
						   0, 0, -2 / fsn, -fan / fsn,
						   0, 0, 0, 1
						   );
	}

};

template< class _T >
struct VWB_MATRIX33
{
    union {
        struct {
            _T        _11, _12, _13;
            _T        _21, _22, _23;
            _T        _31, _32, _33;

        };
        _T m[3][3];
		_T p[9];
    };
	_inline_ VWB_MATRIX33() {};
	_inline_ VWB_MATRIX33( VWB_MATRIX33 const& other ) { memcpy( this, &other, sizeof( *this ) ); }
	_inline_ VWB_MATRIX33( VWB_MATRIX<_T> const& other ) : 
		_11( other._11 ), _12( other._12 ), _13( other._13 ),
		_21( other._21 ), _22( other._22 ), _23( other._23 ), 
		_31( other._31 ), _32( other._32 ), _33( other._33 ) {}
	template<class _T2>
	_inline_ VWB_MATRIX33( VWB_MATRIX33<_T2> const& other ) : 
		_11( (_T)other._11 ), _12( (_T)other._12 ), _13( (_T)other._13 ),
		_21( (_T)other._21 ), _22( (_T)other._22 ), _23( (_T)other._23 ), 
		_31( (_T)other._31 ), _32( (_T)other._32 ), _33( (_T)other._33 ){}
	_inline_ VWB_MATRIX33( _T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_MATRIX33( _T f11, _T f12, _T f13,
			  _T f21, _T f22, _T f23,
			  _T f31, _T f32, _T f33 )
	{
		_11 = f11; _12 = f12; _13 = f13;
		_21 = f21; _22 = f22; _23 = f23;
		_31 = f31; _32 = f32; _33 = f33;
	}
	template< class _T2 >
	_inline_ VWB_MATRIX33& operator=( VWB_MATRIX33<_T2> const& other ) {
		_11 = (_T)other._11; _12 = (_T)other._12; _13 = (_T)other._13;
		_21 = (_T)other._21; _22 = (_T)other._22; _23 = (_T)other._23; 
		_31 = (_T)other._31; _32 = (_T)other._32; _33 = (_T)other._33;
		return *this; }
	_inline_ static VWB_MATRIX33 const& ptr( _T const* p ) { return *(VWB_MATRIX33 const*)p; }
	_inline_ static VWB_MATRIX33& ptr( _T* p ) { return *(VWB_MATRIX33*)p; }
	_inline_ static VWB_MATRIX33 I() { VWB_MATRIX33 res( 1,0,0, 0,1,0, 0,0,1 ); return res;}
	_inline_ static VWB_MATRIX33 O() { VWB_MATRIX33 res; memset( res, 0, sizeof( res ) ); return res;}
	_inline_ static VWB_MATRIX33 S( VWB_VECTOR3<_T> const& scale) { return VWB_MATRIX33( scale.x, 0, 0,  0, scale.y, 0,  0, 0, scale.z );}
	_inline_ static VWB_MATRIX33 S( _T const& sx, _T const& sy, _T const& sz ) { return VWB_MATRIX33( sx, 0, 0,  0, sy, 0,  0, 0, sz );}
	_inline_ static VWB_MATRIX33 Rx( _T x ) { VWB_MATRIX33 res(1,0,0,  0,cos(x),-sin(x),  0,sin(x),cos(x) ); return res; }
	_inline_ static VWB_MATRIX33 Ry( _T y ) { VWB_MATRIX33 res(cos(y),0,-sin(y),  0,1,0, sin(y),0,cos(y) ); return res; } 
	_inline_ static VWB_MATRIX33 Rz( _T z ) { VWB_MATRIX33 res(cos(z),sin(z),0,  -sin(z),cos(z),0, 0,0,1 ); return res; }
	_inline_ static VWB_MATRIX33 R( _T x, _T y, _T z )  // this is Rz(z) * Rx(x) * Ry(y)
	{ 
		return VWB_MATRIX33(
			 cos(y)*cos(z)-sin(x)*sin(y)*sin(z),	cos(x)*sin(z),	-sin(x)*cos(y)*sin(z)-sin(y)*cos(z),
			-sin(x)*sin(y)*cos(z)-cos(y)*sin(z),	 cos(x)*cos(z),	 sin(y)*sin(z)-sin(x)*cos(y)*cos(z),
			 cos(x)*sin(y),							 sin(x),		cos(x)*cos(y)						);
	}
	_inline_ static VWB_MATRIX33 R( VWB_VECTOR3<_T> r )  // this is Rz(z) * Rx(x) * Ry(y)
	{ 
		return VWB_MATRIX33(
			 cos(r.y)*cos(r.z)-sin(r.x)*sin(r.y)*sin(r.z),	cos(r.x)*sin(r.z),	-sin(r.x)*cos(r.y)*sin(r.z)-sin(r.y)*cos(r.z),
			-sin(r.x)*sin(r.y)*cos(r.z)-cos(r.y)*sin(r.z),	 cos(r.x)*cos(r.z),	sin(r.y)*sin(r.z)-sin(r.x)*cos(r.y)*cos(r.z),
			 cos(r.x)*sin(r.y),							 sin(r.x),		cos(r.x)*cos(r.y)						);
	}

	// transposed matrices
	_inline_ static VWB_MATRIX33 RxLHT( _T x ) { VWB_MATRIX33 res( 1,0,0,  0,cos(x),sin(x),  0,-sin(x),cos(x) ); return res; }
	_inline_ static VWB_MATRIX33 RyLHT( _T y ) { VWB_MATRIX33 res(cos(y),0,-sin(y),  0,1,0, sin(y),0,cos(y) ); return res; } 
	_inline_ static VWB_MATRIX33 RzLHT( _T z ) { VWB_MATRIX33 res(cos(z),sin(z),0,  -sin(z),cos(z),0, 0,0,1 ); return res; }
	_inline_ static VWB_MATRIX33 RLHT( _T x, _T y, _T z ) // this is RyT(y) * RxT(x) * RzT(z)
	{ 
		return VWB_MATRIX33( 
			 cos(y)*cos(z)-sin(x)*sin(y)*sin(z),	-sin(x)*sin(y)*cos(z)-cos(y)*sin(z),	-cos(x)*sin(y),
			 cos(x)*sin(z),							cos(x)*cos(z),						 -sin(x),
			 sin(x)*cos(y)*sin(z)+sin(y)*cos(z),	-sin(y)*sin(z)+sin(x)*cos(y)*cos(z),	 cos(x)*cos(y) );
	}
	_inline_ static VWB_MATRIX33 RLHT( VWB_VECTOR3<_T> r) // this is RyT(y) * RxT(x) * RzT(z)
	{ 
		return VWB_MATRIX33( 
			 cos(r.y)*cos(r.z)-sin(r.x)*sin(r.y)*sin(r.z),	-sin(r.x)*sin(r.y)*cos(r.z)-cos(r.y)*sin(r.z),	-cos(r.x)*sin(r.y),
			 cos(r.x)*sin(r.z),							cos(r.x)*cos(r.z),						 -sin(r.x),
			 sin(r.x)*cos(r.y)*sin(r.z)+sin(r.y)*cos(r.z),	-sin(r.y)*sin(r.z)+sin(r.x)*cos(r.y)*cos(r.z),	 cos(r.x)*cos(r.y) );
	}

	// give device x and y axis
	_inline_ static VWB_MATRIX33 base( VWB_VECTOR3<_T> dx, VWB_VECTOR3<_T> dy )
	{
		VWB_MATRIX33 res;
		dx.normalize();
		dy.normalize();
		res._11 = dx.x;
		res._12 = dx.y;
		res._13 = dx.z;
		// y * x gives us a view direction aka base z in right handed system
		res._31 = dx.y*dy.z - dx.z*dy.y;
		res._32 = dx.z*dy.x - dx.x*dy.z;
		res._33 = dx.x*dy.y - dx.y*dy.x;
		// z * x gives us a up aka y
		res._21 = dx.z*res._32 - dx.y*res._33;
		res._22 = dx.x*res._33 - dx.z*res._31;
		res._23 = dx.y*res._31 - dx.x*res._32;
		return res;
	}

	_inline_ _T& operator()(unsigned int row, unsigned int col) { return m[row][col]; }
	_inline_ _T const& operator()(unsigned int row, unsigned int col) const { return m[row][col]; }
	_inline_ operator _T* () { return &_11; }
	_inline_ operator _T const* () const { return &_11; }
	//_inline_ _T* operator[]( int i ) { return m[i]; }
	//_inline_ _T const* operator[]( int i ) const { return m[i]; }

	_inline_ VWB_MATRIX33 operator-() const { return VWB_MATRIX33( -_11, -_12, -_13,
																   -_21, -_22, -_23,
                                                                  -_31, -_32, -_33 ); }
	_inline_ bool operator==(VWB_MATRIX33 const& other) const 
	{ 
		for( int i = 0; i != 9; i++ ) if( p[i] != other.p[i] ) return false;
		return true; 
	}
	_inline_ bool operator!=(VWB_MATRIX33 const& other) const 
	{ 
		for( int i = 0; i != 9; i++ ) if( p[i] != other.p[i] ) return true;
		return false; 
	}

	_inline_ VWB_MATRIX33 operator*( _T const other ) const
	{
		return VWB_MATRIX33( _11 * other, _12 * other, _13 * other,
					  _21 * other, _22 * other, _23 * other,
					  _31 * other, _32 * other, _33 * other);
	}
	_inline_ VWB_MATRIX33& operator*=( _T const other )
	{
		_11*= other; _12*= other; _13*= other;
		_21*= other; _22*= other; _23*= other;
		_31*= other; _32*= other; _33*= other;
		return *this;
	}
	_inline_ VWB_MATRIX33& operator*=( VWB_MATRIX33 const& other )
	{
		VWB_MATRIX33 M = *this;
		*this = M * other;
		return *this;
	}
	_inline_ VWB_MATRIX33 operator*( VWB_MATRIX33 const& other ) const
	{
		VWB_MATRIX33 res;
		for( int i = 0; i != 3; i++ )
			for( int j = 0; j != 3; j++ )
			{
				res.m[i][j] = m[i][0] * other.m[0][j];
				for( int k = 1; k != 3; k++ )
					res.m[i][j]+= m[i][k] * other.m[k][j];
			}
		return res;
	}
	_inline_ VWB_VECTOR3<_T> operator*( VWB_VECTOR3<_T> const& other ) const
	{
		return VWB_VECTOR3<_T>(
			_11 * other.x + _12 * other.y + _13 * other.z,
			_21 * other.x + _22 * other.y + _23 * other.z,
			_31 * other.x + _32 * other.y + _33 * other.z );
	}

	_inline_ void SetPtr( _T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}

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

	_inline_ VWB_MATRIX33& Transpose()
	{
		_T a = _12;
		_12 = _21;
		_21 = a;
		
		a = _13;
		_13 = _31;
		_31 = a;

		a = _23;
		_23 = _32;
		_32 = a;

		return *this;
	}

	_inline_ VWB_MATRIX33 Transposed()	{ return VWB_MATRIX33 ( _11, _21, _31,  _12, _22, _32,  _13, _23, _33 ); }

	_inline_ VWB_MATRIX33& Invert()
	{
		*this = Inverted();
		return *this;
	}

	_inline_ VWB_MATRIX33 Inverted() const
	{
		VWB_MATRIX33 pA;
		_T x = p[4] * p[8] - p[5] * p[7];
		_T y = p[3] * p[8] - p[5] * p[6];
		_T z = p[3] * p[7] - p[4] * p[6];
		_T det = p[0] * x - p[1] * y + p[2] * z;

		det = ((_T)1)/det;

		pA[0] = x * det;
		pA[1] = -( p[1] * p[8] - p[2] * p[7] ) * det;
		pA[2] = ( p[1] * p[5] - p[2] * p[4] ) * det;
		pA[3] = -y * det;
		pA[4] = ( p[0] * p[8] - p[2] * p[6] ) * det;
		pA[5] = -( p[0] * p[5] - p[2] * p[3] ) * det;
		pA[6] = z * det;
		pA[7] = -( p[0] * p[7] - p[1] * p[6] ) * det;
		pA[8] = ( p[0] * p[4] - p[3] * p[1] ) * det;

		return pA;
	}
	_inline_ VWB_MATRIX33& normalize()
	{
		_T l = _11 * _11;
		for( int i = 1; i != 9; i++ )
			l+= p[i] * p[i];
		l = sqrt(l/3);
		if( FLT_MIN < l || -FLT_MIN > l )
		{
			for( int i = 0; i != 9; i++ )
				p[i]/= l;
		}
	return *this;
	}
	_inline_ VWB_VECTOR3<_T>& X() { return *(VWB_VECTOR3<_T>*)&_11; }
	_inline_ VWB_VECTOR3<_T> const& X() const { return *(VWB_VECTOR3<_T>*)&_11; }
	_inline_ VWB_VECTOR3<_T>& Y() { return *(VWB_VECTOR3<_T>*)&_21; }
	_inline_ VWB_VECTOR3<_T> const& Y() const { return *(VWB_VECTOR3<_T>*)&_21; }
	_inline_ VWB_VECTOR3<_T>& Z() { return *(VWB_VECTOR3<_T>*)&_31; }
	_inline_ VWB_VECTOR3<_T> const& Z() const { return *(VWB_VECTOR3<_T>*)&_31; }
};

template< class T >
struct VWB_BOX
{
	VWB_VECTOR3<T> tl, br;
	_inline_ VWB_BOX(){}
	_inline_ VWB_BOX( VWB_BOX const& other ) : tl( other.tl ), br( other.br ) {}
	_inline_ VWB_BOX( VWB_VECTOR3<T> const& _tl, VWB_VECTOR3<T> const& _br ) : tl( _tl ), br( _br ) {}
	_inline_ VWB_BOX& operator+=( VWB_BOX const& other )
	{
		if( other.tl[0] < tl[0] )
			tl[0] = other.tl[0];
		if( other.br[0] > br[0] )
			br[0] = other.br[0];

		if( other.tl[1] < tl[1] )
			tl[1] = other.tl[1];
		if( other.br[1] > br[1] )
			br[1] = other.br[1];

		if( other.tl[2] < tl[2] )
			tl[2] = other.tl[2];
		if( other.br[2] > br[2] )
			br[2] = other.br[2];
		return *this;
	};
	_inline_ VWB_BOX& operator+=( VWB_VECTOR3<T> const& other )
	{
		if( other[0] < tl[0] )
			tl[0] = other[0];
		if( other[0] > br[0] )
			br[0] = other[0];

		if( other[1] < tl[1] )
			tl[1] = other[1];
		if( other[1] > br[1] )
			br[1] = other[1];

		if( other[2] < tl[2] )
			tl[2] = other[2];
		if( other[2] > br[2] )
			br[2] = other[2];
		return *this;
	};
	_inline_ VWB_BOX operator+( VWB_BOX const& other ) const
	{
		VWB_BOX r( *this );
		if( other.tl[0] < r.tl[0] )
			r.tl[0] = other.r.tl[0];
		if( other.br[0] > r.br[0] )
			r.br[0] = other.br[0];

		if( other.tl[1] < r.tl[1] )
			r.tl[1] = other[1];
		if( other.br[1] > r.br[1] )
			r.br[1] = other.br[1];

		if( other.tl[2] < r.tl[2] )
			r.tl[2] = other.tl[2];
		if( other.br[2] > r.br[2] )
			r.br[2] = other.br[2];
		return r;
	};

	_inline_ VWB_BOX operator+( VWB_VECTOR3<T> const& other ) const
	{
		VWB_BOX r( *this );
		if( other[0] < r.r.tl[0] )
			r.tl[0] = other[0];
		if( other[0] > r.tl[0] )
			r.tl[0] = other[0];

		if( other[1] < r.tl[1] )
			r.tl[1] = other[1];
		if( other[1] > r.tl[1] )
			r.tl[1] = other[1];

		if( other[2] < r.tl[2] )
			r.tl[2] = other[2];
		if( other[2] > r.br[2] )
			r.br[2] = other[2];
		return r;
	};
	_inline_ bool IsEmpty() { return tl.x < br.x && tl.y < br.y && tl.z < br.z; }
	_inline_ static VWB_BOX M() { return VWB_BOX( VWB_VECTOR3<T>( FLT_MAX,FLT_MAX,FLT_MAX ), VWB_VECTOR3<T>( -FLT_MAX,-FLT_MAX,-FLT_MAX ) ); }
	_inline_ static VWB_BOX O() { return VWB_BOX( VWB_VECTOR3<T>( 0,0,0 ), VWB_VECTOR3<T>( 0,0,0 ) ); }
};

#pragma pack(pop)

typedef VWB_MATRIX33<VWB_float> VWB_MAT33f;
typedef VWB_VECTOR3<VWB_float> VWB_VEC3f;

typedef VWB_MATRIX<VWB_float> VWB_MAT44f;
typedef VWB_VECTOR4<VWB_float> VWB_VEC4f;

typedef VWB_BOX<VWB_float> VWB_BOXf;

typedef VWB_MATRIX33<VWB_double> VWB_MAT33d;
typedef VWB_VECTOR3<VWB_double> VWB_VEC3d;

typedef VWB_MATRIX<VWB_double> VWB_MAT44d;
typedef VWB_VECTOR4<VWB_double> VWB_VEC4d;

typedef VWB_BOX<VWB_double> VWB_BOXd;

#endif
