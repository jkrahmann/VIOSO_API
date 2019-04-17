#include "DXWarpBlend.h"
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxerr.lib" )

DXWarpBlend::DXWarpBlend()
: VWB_Warper_base()
{
	m_modelPath[0] = 0;
}

DXWarpBlend::~DXWarpBlend()
{
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR DXWarpBlend::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
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
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

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

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR DXWarpBlend::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
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

VWB_ERROR DXWarpBlend::SetViewProjection( VWB_float const* pView, VWB_float const* pProj )
{
	if( pView && pProj )
		m_mVP = m_mBaseI * VWB_MAT44f::ptr( pView ) * VWB_MAT44f::ptr( pProj );
	else
		return VWB_ERROR_PARAMETER;
	return VWB_ERROR_NONE;
}
