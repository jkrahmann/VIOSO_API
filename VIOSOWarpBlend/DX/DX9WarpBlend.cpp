#include "DX9WarpBlend.h"
#include "pixelshader.h"

#pragma comment( lib, "d3d9.lib" )
//#pragma comment( lib, "d3dx9.lib" )

#if defined( VWB_USE_DEPRECATED_INIT )
VWB_DX9WHANDLE VWB_InitDX9A( LPDIRECT3DDEVICE9 pDevice, LPCSTR szConfigFile, LPCSTR szChannel )
{
	VWB_Warper* pW = NULL;
	g_error = (VWB_int)VWB_CreateA( pDevice, szConfigFile, szChannel, &pW, 1, NULL );
	if( 0 == g_error )
		g_error = (VWB_int)VWB_Init( pW );
	if( 0 == g_error )
		return pW;
	VWB_Destroy( pW );
	return NULL;
}

VWB_DX9WHANDLE VWB_InitDX9W( LPDIRECT3DDEVICE9 pDevice, LPCWSTR szConfigFile, LPCWSTR szChannel )
{
	VWB_Warper* pW = NULL;
	g_error = (VWB_int)VWB_CreateW( pDevice, szConfigFile, szChannel, &pW, 1, NULL );
	if( VWB_ERROR_NONE == g_error )
		g_error = (VWB_int)VWB_Init( pW );
	if( VWB_ERROR_NONE == g_error )
		return pW;
	VWB_Destroy( pW );
	return NULL;
}

void VWB_DestroyDX9( VWB_DX9WHANDLE h )
{
	VWB_Destroy( h );
}

BOOL VWB_updateViewProjDX9( VWB_DX9WHANDLE h, D3DXVECTOR4* eye, D3DXMATRIX* pView, D3DXMATRIX* pProj )
{
	return VWB_ERROR_NONE == VWB_getViewProj( h, *eye, NULL, *pView, *pProj );
}

BOOL VWB_updateViewProj2DX9( VWB_DX9WHANDLE h, D3DXVECTOR4* eye, D3DXVECTOR4* rot, D3DXMATRIX* pView, D3DXMATRIX* pProj )
{
	return VWB_ERROR_NONE == VWB_getViewProj( h, *eye, *rot, *pView, *pProj );
}

HRESULT VWB_renderDX9( VWB_DX9WHANDLE h, LPDIRECT3DTEXTURE9 pSrc )
{
	return VWB_ERROR_NONE == VWB_render( h, (VWB_param)pSrc, VWB_STATEMASK_STANDARD ) ? S_OK : E_FAIL;
}

#endif //defined( VWB_USE_DEPRECATED_INIT )
////////////////////////////////////////////////////////////////////////////////////////////

const DWORD DXSCREENVERTEX::FVF = D3DFVF_XYZ | D3DFVF_TEX1;

DX9WarpBlend::DX9WarpBlend( LPDIRECT3DDEVICE9 pDevice )
	: DXWarpBlend(),
	m_device( pDevice ),
	m_PixelShader( NULL ),
	m_VertexBuffer( NULL ),
	m_VertexBufferModel( NULL ),
	m_texBlend( NULL ),
	m_texWarp( NULL ),
	m_texBB( NULL ),
	m_srfBB( NULL ),
	m_texWarpCalc( NULL ),
	m_texCur( nullptr ),
	m_vp( { 0 } )
{
	if( NULL == m_device )
		throw( VWB_ERROR_PARAMETER );
	else
		m_device->AddRef();
	m_type4cc = '\09XD';
}

DX9WarpBlend::~DX9WarpBlend( void )
{
	SAFERELEASE( m_texWarpCalc );
	SAFERELEASE( m_texCur );
	SAFERELEASE( m_texBB );
	SAFERELEASE( m_srfBB );
	SAFERELEASE( m_texWarp );
	SAFERELEASE( m_texBlend );
	SAFERELEASE( m_PixelShader );
	SAFERELEASE( m_VertexBuffer );
	SAFERELEASE( m_device );
	logStr( 1, "INFO: DX9-Warper destroyed.\n" );
}


VWB_ERROR DX9WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	if( VWB_ERROR_NONE == err ) try
	{
		VWB_WarpBlend& wb = *wbs[calibIndex];
		D3DDEVICE_CREATION_PARAMETERS p = { 0 };

		RECT rC = { 0 };
		if( FAILED( m_device->GetCreationParameters( &p ) ) ||
			0 == p.hFocusWindow ||
			!::GetClientRect( p.hFocusWindow, &rC ) )
		{
			logStr( 1, "WARNING: No window found, cannot set viewport.\n" );
			::memset( &m_vp, 0, sizeof( m_vp ) );
		}
		else
		{
			m_vp.Width = rC.right;
			m_vp.Height = rC.bottom;
			m_vp.X = 0;
			m_vp.Y = 0;
			m_vp.MinZ = 0.0f;
			m_vp.MaxZ = 1.0f;
			logStr( 2, "Window found. Viewport is %ux%u.\n", m_vp.Width, m_vp.Height );
		}

		if( FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? D3DFMT_A32B32G32R32F : D3DFMT_G32R32F, D3DPOOL_MANAGED, &m_texBlend, NULL ) ) ||
			FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, D3DFMT_A16B16G16R16, D3DPOOL_MANAGED, &m_texWarp, NULL ) ) )
		{
			logStr( 0, "ERROR: Failed to create lookup textures.\n" );
			throw VWB_ERROR_SHADER;
		}

		D3DLOCKED_RECT r;
		if( FAILED( m_texWarp->LockRect( 0, &r, NULL, 0 ) ) )
		{
			logStr( 0, "ERROR: Failed to fill warp texture.\n" );
			throw VWB_ERROR_WARP;
		}
		if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
		{
			memcpy( r.pBits, wb.pWarp, sizeof( *wb.pWarp ) * m_sizeMap.cx * m_sizeMap.cy );
		}
		else
		{
			for( float* d = (float*)r.pBits, *s = (float*)wb.pWarp, *sE = ( (float*)wb.pWarp ) + m_sizeMap.cx * m_sizeMap.cy; s != sE; d += 2, s += 4 )
			{
				d[0] = s[0];
				d[1] = s[1];
			}
		}
		m_texWarp->UnlockRect( 0 );

		if( FAILED( m_texBlend->LockRect( 0, &r, NULL, 0 ) ) )
		{
			logStr( 0, "ERROR: Failed to fill blend texture.\n" );
			throw VWB_ERROR_BLEND;
		}
		memcpy( r.pBits, wb.pBlend2, sizeof( *wb.pBlend2 ) * m_sizeMap.cx * m_sizeMap.cy );
		m_texBlend->UnlockRect( 0 );

		std::string pixelShader = "PS"; // or "TST"
		if( m_bDynamicEye )
		{
			pixelShader = "PSWB3D";
		}
		else
		{
			pixelShader = "PSWB";
		}
		if( bBicubic )
			pixelShader.append( "BC" );

		ID3DBlob* pCode = NULL;
		ID3DBlob* pErr = NULL;
		if( SUCCEEDED( D3DCompile( s_pixelShaderDX2a, sizeof( s_pixelShaderDX2a ), NULL, NULL, NULL, pixelShader.c_str(), "ps_2_a", 0, 0, &pCode, &pErr ) ) )
		{
			HRESULT hr = m_device->CreatePixelShader( (DWORD*)pCode->GetBufferPointer(), &m_PixelShader );
			SAFERELEASE( pCode );
			SAFERELEASE( pErr );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Failed to create shader!\n" );
				throw VWB_ERROR_SHADER;
			}
			logStr( 2, "INFO: Pixelshader created.\n" );
		}
		else
		{
			if( pErr )
			{
				logStr( 0, "ERROR: Failed to compile shader %s!\n", (char*)pErr->GetBufferPointer() );
				pErr->Release();
			}
			throw VWB_ERROR_SHADER;
		}

		m_VertexBuffer = CreateVertexBuffer( static_cast<float>( m_sizeMap.cx ), static_cast<float>( m_sizeMap.cy ) );
		if( !m_VertexBuffer )
		{
			logStr( 0, "ERROR: Failed to create vertex buffer.\n" );
			throw VWB_ERROR_SHADER;
		}
		logStr( 1, "SUCCESS: DX9-Warper initialized.\n" );

		// transpose view matrices
		m_mBaseI.Transpose();
		m_mViewIG.Transpose();
	}
	catch( VWB_ERROR e )
	{
		err = e;
	}
	return err;
}

VWB_ERROR DX9WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	HRESULT res;
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT;

	if( NULL == m_PixelShader || NULL == m_device )
		return VWB_ERROR_GENERIC;

	// set viewport

	LPDIRECT3DTEXTURE9 pSrc;
	// do backbuffer copy if necessary
	if( NULL == inputTexture ||
		VWB_UNDEFINED_GL_TEXTURE == inputTexture )
	{
		LPDIRECT3DSURFACE9 srfBB = NULL;
		if( SUCCEEDED( m_device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &srfBB ) ) )
		{
			D3DSURFACE_DESC desc;
			srfBB->GetDesc( &desc );
			if( m_sizeIn.cx != desc.Width || m_sizeIn.cy != desc.Height )
				SAFERELEASE( m_texBB );

			if( NULL == m_texBB )
			{
				SAFERELEASE( m_srfBB );
				if( FAILED( m_device->CreateTexture( desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, desc.Pool, &m_texBB, NULL ) ) ||
					FAILED( m_texBB->GetSurfaceLevel( 0, &m_srfBB ) ) )
					return VWB_ERROR_GENERIC;
			}
			res = m_device->StretchRect( srfBB, NULL, m_srfBB, NULL, D3DTEXF_NONE );
			SAFERELEASE( srfBB );
			if( FAILED( res ) )
				return VWB_ERROR_GENERIC;
		}
		else
			return VWB_ERROR_GENERIC;

		pSrc = m_texBB;
	}
	else
		pSrc = (LPDIRECT3DTEXTURE9)inputTexture;

	if( bBicubic && ( 0 == m_sizeIn.cx || 0 == m_sizeIn.cy ) )
	{
		D3DSURFACE_DESC desc;
		pSrc->GetLevelDesc( 0, &desc );
		m_sizeIn.cx = desc.Width;
		m_sizeIn.cy = desc.Height;
	}

	IDirect3DVertexBuffer9* pOldVtx = NULL;
	UINT oldOffset, oldStride;
	DWORD oldFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
	LPDIRECT3DSTATEBLOCK9 restoreState = NULL;
	IDirect3DVertexShader9* pOldVS = NULL;
	IDirect3DPixelShader9* pOldPS = NULL;
	float oldFVals[24] = { 0 };
	IDirect3DBaseTexture9* ppOldTex[4] = { NULL };
	D3DMATRIX mOldView, mOldWorld, mOldProj;

	// Record rendering state

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		m_device->GetStreamSource( 0, &pOldVtx, &oldOffset, &oldStride );
	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		m_device->GetFVF( &oldFVF );

	if( ( VWB_STATEMASK_RASTERSTATE | VWB_STATEMASK_SAMPLER ) & stateMask )
		m_device->CreateStateBlock( D3DSBT_ALL, &restoreState );

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		m_device->GetVertexShader( &pOldVS );
	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		m_device->GetPixelShader( &pOldPS );

	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		m_device->GetPixelShader( &pOldPS );

	if( VWB_STATEMASK_CONSTANT_BUFFER & stateMask )
	{
		m_device->GetPixelShaderConstantF( 0, oldFVals, 6 );
		m_device->GetTransform( D3DTS_WORLD, &mOldWorld );
		m_device->GetTransform( D3DTS_VIEW, &mOldView );
		m_device->GetTransform( D3DTS_PROJECTION, &mOldProj );
	}
	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		m_device->GetTexture( 0, &ppOldTex[0] );
		m_device->GetTexture( 1, &ppOldTex[1] );
		m_device->GetTexture( 2, &ppOldTex[2] );
		m_device->GetTexture( 3, &ppOldTex[3] );
	}
	// Set rendering state
	res = m_device->SetRenderState( D3DRS_AMBIENT, RGB( 255, 255, 255 ) );
	res = m_device->SetRenderState( D3DRS_LIGHTING, FALSE );
	res = m_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	res = m_device->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );

	// Clear the current rendering target
	res = m_device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 0, 0 );

	// Set the shaders
	res = m_device->SetPixelShader( m_PixelShader );
	res = m_device->SetVertexShader( NULL );     // In shader model 2.0 a vertex shader is optional. Necessary for 3.0 though.

	// Set the texturing modes
	SetTexture( 0, pSrc );
	res = m_device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_GAUSSIANQUAD ); // GAUSSIANQUAD is a little better as it preserves fine structures, while sampling in the source texture
	res = m_device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_GAUSSIANQUAD );
	SetTexture( 1, m_texWarp );
	SetTexture( 2, m_texBlend );

	if( m_bDynamicEye )
	{
		// on opposite to ID3DXEffect::SetMatrix, which uses row-major order, a flat copy via IDirect3DDevice9::SetPixelShaderConstantF uses flat copy which is column-major order
		m_device->SetPixelShaderConstantF( 0, m_mVP.Transposed(), 4 );
	}
	// SetPixelShaderConstantB (and boolean registers) are not available in shader model 2_0
	float vals[4] = {
		m_bBorder ? 1.0f : 0.0f,
		bDoNotBlend ? 0.0f : 1.0f,
		0, 0 };
	m_device->SetPixelShaderConstantF( 4, vals, 1 );

	if( bBicubic )
	{
		float vals[4] = { (float)m_sizeIn.cx, (float)m_sizeIn.cy, 1.0f / (float)m_sizeIn.cx, 1.0f / (float)m_sizeIn.cy };
		m_device->SetPixelShaderConstantF( 5, vals, 1 );
	}

	if( bPartialInput )
	{
		float vals[4] = {
			(float)optimalRect.left / (float)optimalRes.cx,
			(float)optimalRect.top / (float)optimalRes.cy,
			(float)optimalRes.cx / ( (float)optimalRect.right - (float)optimalRect.left ),
			(float)optimalRes.cy / ( (float)optimalRect.bottom - (float)optimalRect.top )
		};
		m_device->SetPixelShaderConstantF( 6, vals, 1 );
	}
	else
	{
		float vals[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		m_device->SetPixelShaderConstantF( 6, vals, 1 );
	}

	if( mouseMode & 1 )
	{
		CURSORINFO ci = { 0 };
		ci.cbSize = sizeof( ci );
		::GetCursorInfo( &ci );

		// check if sizes of cursor and cursor texture differs
		if( ci.hCursor && ( g_hCur != ci.hCursor || nullptr == m_texCur ) )
		{
			g_hCur = ci.hCursor;
			ICONINFO ii = { 0 };

			if( ::GetIconInfo( ci.hCursor, &ii ) && ii.hbmMask )
			{
				BITMAP bmMask = { 0 };
				BITMAP bmColor = { 0 };
				::GetObjectW( ii.hbmMask, sizeof( BITMAP ), &bmMask );
				if( ii.hbmColor )
				{
					::GetObjectW( ii.hbmColor, sizeof( BITMAP ), &bmColor );
				}

				if( nullptr != m_texCur && g_dimCur.cx != static_cast<UINT>( bmMask.bmWidth ) )
				{
					SAFERELEASE( m_texCur );
				}

				if( nullptr == m_texCur && bmMask.bmWidth )
				{ // create an appropriate texture object
					g_dimCur.cx = bmMask.bmWidth;
					g_dimCur.cy = bmColor.bmHeight ? bmColor.bmHeight : bmMask.bmHeight / 2;
					g_hotCur.x = ii.xHotspot;
					g_hotCur.y = ii.yHotspot;

					if( FAILED( m_device->CreateTexture( g_dimCur.cx, g_dimCur.cy, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_texCur, NULL ) ) )
					{
						logStr( 0, "WARNING: Failed to create mouse cursor textures. Mouse rendering disabled.\n" );
						mouseMode &= ~1;
					}
				}

				D3DLOCKED_RECT r;
				if( nullptr != m_texCur &&
					SUCCEEDED( m_texCur->LockRect( 0, &r, NULL, 0 ) ) )
				{
					copyCursorBitmapToMappedTexture( ii.hbmMask, ii.hbmColor, bmMask, bmColor, r.pBits, r.Pitch );
					m_texCur->UnlockRect( 0 );
				}
				else
				{
					logStr( 0, "WARNING: Failed to fill mouse texture. Mouse rendering disabled.\n" );
					mouseMode &= ~1;
					SAFERELEASE( m_texCur );
				}
				::DeleteObject( ii.hbmColor );
				::DeleteObject( ii.hbmMask );
			}
		}

		D3DDEVICE_CREATION_PARAMETERS dcp = { 0 };
		if( S_OK == m_device->GetCreationParameters( &dcp ) &&
			0 != dcp.hFocusWindow )
		{
			RECT rWnd = { 0 };
			if( GetWindowRect( dcp.hFocusWindow, &rWnd ) )
			{
				if( nullptr != ShowSystemCursor &&
					0 != ( mouseMode & 2 ) )
				{
					if( PtInRect( &rWnd, ci.ptScreenPos ) )
					{
						if( g_bCurEnabled )
						{
							ShowSystemCursor( FALSE );
							g_bCurEnabled = false;
						}
					}
					else
					{
						if( !g_bCurEnabled )
						{
							ShowSystemCursor( TRUE );
							g_bCurEnabled = true;
						}
					}
				}
			}

			float vals[4] = {
				static_cast<float>( ci.ptScreenPos.x - g_hotCur.x * 4 / 3 - rWnd.left ) / static_cast<float>( rWnd.right - rWnd.left ),
				static_cast<float>( ci.ptScreenPos.y - g_hotCur.y * 4 / 3 - rWnd.top ) / static_cast<float>( rWnd.bottom - rWnd.top ),
				static_cast<float>( rWnd.right - rWnd.left ) / static_cast<float>( g_dimCur.cx * 4 / 3 ),
				static_cast<float>( rWnd.bottom - rWnd.top ) / static_cast<float>( g_dimCur.cx * 4 / 3 )
			};
			m_device->SetPixelShaderConstantF( 7, vals, 1 );
		}
		else
		{
			float vals[4] = { -2.0f, -2.0f, 1.0f, 1.0f };
			m_device->SetPixelShaderConstantF( 7, vals, 1 );
		}
	}
	else
	{
		float vals[4] = { -2.0f, -2.0f, 1.0f, 1.0f };
		m_device->SetPixelShaderConstantF( 7, vals, 1 );
	}
	SetTexture( 3, m_texCur );

	// Set ortho projection filling the viewport
	D3DXMATRIX lOrthoMatrix;
	D3DXMatrixOrthoOffCenterLH( &lOrthoMatrix, 0.0, static_cast<float>( m_sizeMap.cx ), 0.0, static_cast<float>( m_sizeMap.cy ), 0.0, 1.0 );
	res = m_device->SetTransform( D3DTS_PROJECTION, &lOrthoMatrix );

	// Set Identity view and world matrix
	D3DXMATRIX lViewMatrix;
	D3DXMatrixIdentity( &lViewMatrix );
	res = m_device->SetTransform( D3DTS_VIEW, &lViewMatrix );
	D3DXMATRIX lWorldMatrix;
	D3DXMatrixIdentity( &lWorldMatrix );
	res = m_device->SetTransform( D3DTS_WORLD, &lWorldMatrix );

	// Do the rendering to the current render target
	res = m_device->SetFVF( DXSCREENVERTEX::FVF );
	res = m_device->SetStreamSource( 0, m_VertexBuffer, 0, sizeof( DXSCREENVERTEX ) );
	res = m_device->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

	// Restore rendering state

	if( ( VWB_STATEMASK_SAMPLER | VWB_STATEMASK_RASTERSTATE ) & stateMask )
		if( restoreState )
		{
			res = restoreState->Apply();
			restoreState->Release();
		}

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		if( ppOldTex[0] )
		{
			m_device->SetTexture( 0, ppOldTex[0] );
			ppOldTex[0]->Release();
		}
		if( ppOldTex[1] )
		{
			m_device->SetTexture( 1, ppOldTex[1] );
			ppOldTex[1]->Release();
		}
		if( ppOldTex[2] )
		{
			m_device->SetTexture( 2, ppOldTex[2] );
			ppOldTex[2]->Release();
		}
		if( ppOldTex[3] )
		{
			m_device->SetTexture( 3, ppOldTex[3] );
			ppOldTex[3]->Release();
		}
	}

	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		if( pOldPS )
		{
			m_device->SetPixelShader( pOldPS );
			pOldPS->Release();
		}

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		if( pOldVS )
		{
			m_device->SetVertexShader( pOldVS );
			pOldVS->Release();
		}

	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		m_device->SetFVF( oldFVF );

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		if( pOldVtx )
		{
			m_device->SetStreamSource( 0, pOldVtx, oldOffset, oldStride );
			pOldVtx->Release();
		}

	return VWB_ERROR_NONE;
}

//--------------------------------------------------------------------------
//
// Utility Functions
//

LPDIRECT3DVERTEXBUFFER9 DX9WarpBlend::CreateVertexBuffer( float width, float height ) const
{
	LPDIRECT3DVERTEXBUFFER9 buf;
	m_device->CreateVertexBuffer( sizeof( DXSCREENVERTEX ) * 4, D3DUSAGE_WRITEONLY, DXSCREENVERTEX::FVF, D3DPOOL_MANAGED, &buf, NULL );

	DXSCREENVERTEX *quad = NULL;
	buf->Lock( 0, 0, (void**)&quad, 0 );

	quad[0].pos = D3DXVECTOR3( -0.5f, height + 0.5f, 0.0f );
	quad[1].pos = D3DXVECTOR3( width - 0.5f, height + 0.5f, 0.0f );
	quad[2].pos = D3DXVECTOR3( -0.5f, 0.5f, 0.0f );
	quad[3].pos = D3DXVECTOR3( width - 0.5f, 0.5f, 0.0f );
	quad[0].tex1 = D3DXVECTOR2( 0.0f, 0.0f );
	quad[1].tex1 = D3DXVECTOR2( 1.0f, 0.0f );
	quad[2].tex1 = D3DXVECTOR2( 0.0f, 1.0f );
	quad[3].tex1 = D3DXVECTOR2( 1.0f, 1.0f );

	buf->Unlock();

	return buf;
}

void DX9WarpBlend::SetTexture( unsigned int texId, LPDIRECT3DTEXTURE9 tex ) const
{
	HRESULT res;
	res = m_device->SetTexture( texId, tex );
	if( NULL != tex )
	{
		res = m_device->SetTextureStageState( texId, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		res = m_device->SetTextureStageState( texId, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		res = m_device->SetTextureStageState( texId, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		res = m_device->SetSamplerState( texId, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER );
		res = m_device->SetSamplerState( texId, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER );
		res = m_device->SetSamplerState( texId, D3DSAMP_BORDERCOLOR, D3DCOLOR_RGBA( 0, 0, 0, 0 ) );
		res = m_device->SetSamplerState( texId, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		res = m_device->SetSamplerState( texId, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		res = m_device->SetSamplerState( texId, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		res = m_device->SetSamplerState( texId, D3DSAMP_SRGBTEXTURE, 0 );
	}
}
