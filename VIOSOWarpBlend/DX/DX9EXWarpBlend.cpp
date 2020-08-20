#include "DX9EXWarpBlend.h"
#include "pixelshader.h"

#pragma comment( lib, "d3d9.lib" )

//const DWORD DXSCREENVERTEX::FVF = D3DFVF_XYZ | D3DFVF_TEX1; // already initialized in D3D9WarpBlend.cpp !

DX9EXWarpBlend::DX9EXWarpBlend( LPDIRECT3DDEVICE9EX pDevice )
	: DXWarpBlend(),
	m_device( pDevice ),
	m_PixelShader(NULL),
	m_VertexBuffer(NULL),
//	m_VertexBufferModel(NULL),
	m_texBlend( NULL ),
	m_texBlack( NULL ),
	m_texWarp(NULL),
	m_texBB(NULL),
	m_srfBB(NULL),
	m_texWarpCalc(NULL),
	m_texCur( nullptr ),
	m_texCurSysMem( nullptr ),
	m_vp( { 0 } )
{
	if( NULL == m_device )
		throw( VWB_ERROR_PARAMETER );
	else
		m_device->AddRef();
	m_type4cc = 'X9XD';
}

DX9EXWarpBlend::~DX9EXWarpBlend(void)
{
	SAFERELEASE( m_texWarpCalc );
	SAFERELEASE( m_texCur );
	SAFERELEASE( m_texCurSysMem );
	SAFERELEASE( m_texBB );
	SAFERELEASE( m_srfBB );
	SAFERELEASE( m_texWarp ); 
	SAFERELEASE( m_texBlend );
	SAFERELEASE( m_texBlack );
	SAFERELEASE( m_PixelShader );
	SAFERELEASE( m_VertexBuffer );
	SAFERELEASE( m_device );
	logStr( 1, "INFO: DX9X-Warper destroyed.\n" );
}


VWB_ERROR DX9EXWarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	if( VWB_ERROR_NONE == err ) try
	{
		VWB_WarpBlend& wb = *wbs[calibIndex];
		D3DDEVICE_CREATION_PARAMETERS p = {0};

		RECT rC = {0};
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

		LPDIRECT3DTEXTURE9 texTmpW = NULL;
		LPDIRECT3DTEXTURE9 texTmpB = NULL;
		LPDIRECT3DTEXTURE9 texTmpBl = NULL;
		if ( FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, 0 != ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? D3DFMT_A32B32G32R32F : D3DFMT_G32R32F, D3DPOOL_SYSTEMMEM, &texTmpW, NULL ) ) ||
			 FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, D3DFMT_A16B16G16R16, D3DPOOL_SYSTEMMEM, &texTmpB, NULL ) ) ||
			 FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, D3DFMT_A8B8G8R8, D3DPOOL_SYSTEMMEM, &texTmpBl, NULL ) ) ||
			 FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, 0 != ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? D3DFMT_A32B32G32R32F : D3DFMT_G32R32F, D3DPOOL_DEFAULT, &m_texWarp, NULL ) ) ||
			 FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, &m_texBlend, NULL ) )  ||
			 FAILED( m_device->CreateTexture( m_sizeMap.cx, m_sizeMap.cy, 1, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, &m_texBlack, NULL ) )
			 )
		{
			SAFERELEASE( texTmpW );
			SAFERELEASE( texTmpB );
			SAFERELEASE( texTmpBl );
			logStr( 0, "ERROR: Failed to create lookup textures.\n" );
			return VWB_ERROR_SHADER;
		}

		const size_t sz = size_t( m_sizeMap.cx ) * m_sizeMap.cy;
		D3DLOCKED_RECT r;

        if( FAILED(texTmpW->LockRect( 0, &r, NULL, 0 ) ) )
		{
			SAFERELEASE(texTmpW);
			logStr( 0, "ERROR: Failed to fill temp texture for blend.\n" );
			return VWB_ERROR_WARP;
		}
		if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
		{
			memcpy( r.pBits, wb.pWarp, sizeof( *wb.pWarp ) * sz );
		}
		else
		{
			float* d = (float*)r.pBits;
			for( VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + sz; s != sE; d += 2, s++ )
			{
				d[0] = s->x;
				d[1] = s->y;
			}
		}
		texTmpW->UnlockRect(0);
		if(FAILED(m_device->UpdateTexture(texTmpW, m_texWarp)))
		{
			SAFERELEASE(texTmpB);
			logStr(0, "ERROR: Failed to update warp texture.\n");
			return VWB_ERROR_WARP;
		}

        if( FAILED(texTmpB->LockRect( 0, &r, NULL, 0 ) ) )
		{
			SAFERELEASE(texTmpB);
			logStr( 0, "ERROR: Failed to fill temp texture for blend.\n" );
			return VWB_ERROR_BLEND;
		}
		memcpy( r.pBits, wb.pBlend2, sizeof( *wb.pBlend2 ) * sz );
		texTmpB->UnlockRect(0);
		if(FAILED(m_device->UpdateTexture(texTmpB, m_texBlend)))
		{
			SAFERELEASE(texTmpB);
				logStr(0, "ERROR: Failed to update warp texture.\n");
				return VWB_ERROR_WARP;
		}

		if( wb.pBlack )
		{
			if( FAILED( texTmpBl->LockRect( 0, &r, NULL, 0 ) ) )
			{
				SAFERELEASE( texTmpBl );
				logStr( 0, "ERROR: Failed to fill temp texture for black level correction.\n" );
				return VWB_ERROR_BLEND;
			}
			memcpy( r.pBits, wb.pBlack, sizeof( *wb.pBlack ) * sz );
			texTmpBl->UnlockRect( 0 );
			if( FAILED( m_device->UpdateTexture( texTmpBl, m_texBlack ) ) )
			{
				SAFERELEASE( texTmpBl );
				logStr( 0, "ERROR: Failed to update warp texture.\n" );
				return VWB_ERROR_WARP;
			}
		}
		SAFERELEASE( texTmpW );
		SAFERELEASE( texTmpB );
		SAFERELEASE( texTmpBl );
		logStr(2, "INFO: Warp, blend and black level lookup maps created.\n");


        std::string pixelShader = "PS"; // or "TST"
        if (m_bDynamicEye)
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
				return VWB_ERROR_SHADER;
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
			return VWB_ERROR_SHADER;
		}

        m_VertexBuffer = CreateVertexBuffer(static_cast<float>(m_sizeMap.cx), static_cast<float>(m_sizeMap.cy));
		if( !m_VertexBuffer)
		{
			logStr( 0, "ERROR: Failed to create vertex buffer.\n" );
			return VWB_ERROR_SHADER;
		}
		logStr( 1, "SUCCESS: DX9-Warper initialized.\n" );

		// transpose view matrices
		m_mBaseI.Transpose();
		m_mViewIG.Transpose();
	} catch( VWB_ERROR e )
	{
		err = e;
	}
	return err;
}

VWB_ERROR DX9EXWarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	logStr( 4, "Render begin." );
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
					FAILED( m_texBB->GetSurfaceLevel(0, &m_srfBB ) ) )
					return VWB_ERROR_GENERIC;
			}
			res = m_device->StretchRect( srfBB, NULL, m_srfBB, NULL, D3DTEXF_NONE );
			SAFERELEASE( srfBB );
			if( FAILED( res ) )
				return VWB_ERROR_GENERIC;
			logStr( 4, "Backbuffer copied." );
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
	UINT oldOffset,oldStride;
	DWORD oldFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    LPDIRECT3DSTATEBLOCK9 restoreState = NULL;
	IDirect3DVertexShader9* pOldVS = NULL;
	IDirect3DPixelShader9* pOldPS = NULL;
	float oldFVals[24] = {0};
	IDirect3DBaseTexture9* ppOldTex[5] = {NULL};
	D3DMATRIX mOldView,mOldWorld,mOldProj;

    // Record rendering state

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		m_device->GetStreamSource( 0, &pOldVtx, &oldOffset, &oldStride );
	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		m_device->GetFVF( &oldFVF );

	if( ( VWB_STATEMASK_RASTERSTATE| VWB_STATEMASK_SAMPLER ) & stateMask )
	    m_device->CreateStateBlock(D3DSBT_ALL, &restoreState);

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
		for( DWORD i = 0; i != ARRAYSIZE( ppOldTex ); i++ )
			m_device->GetTexture( i, &ppOldTex[i] );
	}

	// Set rendering state
    res = m_device->SetRenderState(D3DRS_AMBIENT,RGB(255,255,255));
    res = m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    res = m_device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    res = m_device->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);

    // Clear the current rendering target
	if( stateMask & VWB_STATEMASK_CLEARBACKBUFFER )
		res = m_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 0, 0);

	// Set the shaders
	res = m_device->SetPixelShader( m_PixelShader );
	res = m_device->SetVertexShader( NULL );     // In shader model 2.0 a vertex shader is optional. Necessary for 3.0 though.

	// Set the texturing modes
    SetTexture(0, pSrc);
    res = m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_GAUSSIANQUAD); // GAUSSIANQUAD is a little better as it preserves fine structures, while sampling in the source texture
    res = m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_GAUSSIANQUAD);
    SetTexture(1, m_texWarp);
	SetTexture( 2, m_texBlend );

	if (m_bDynamicEye)
    {
		// on opposite to ID3DXEffect::SetMatrix, which uses row-major order, a flat copy via IDirect3DDevice9::SetPixelShaderConstantF uses flat copy which is column-major order
        m_device->SetPixelShaderConstantF(0, m_mVP.Transposed(), 4);
    }
    // SetPixelShaderConstantB (and boolean registers) are not available in shader model 2_0
	float vals[4] = {
		m_bBorder ? 1.0f : 0.0f,
		bDoNotBlend ? 0.0f : 1.0f,
		bDoNoBlack ? 0.0f : 1.0f,
		0 };
    m_device->SetPixelShaderConstantF( 4, vals, 1 );

	if( bBicubic )
	{
		float vals[4] = { (float)m_sizeIn.cx, (float)m_sizeIn.cy, 1.0f/(float)m_sizeIn.cx, 1.0f/(float)m_sizeIn.cy };
		m_device->SetPixelShaderConstantF( 5, vals, 1 );
	}

	if( bPartialInput )
	{
		float vals[4] = { 
			(float)optimalRect.left / (float)optimalRes.cx,
			(float)optimalRect.top / (float)optimalRes.cy,
			((float)optimalRect.right - (float)optimalRect.left ) / (float)optimalRes.cx,
			((float)optimalRect.bottom - (float)optimalRect.top ) / (float)optimalRes.cy 
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
					SAFERELEASE( m_texCurSysMem );
				}

				if( nullptr == m_texCur && bmMask.bmWidth )
				{ // create an appropriate texture object
					g_dimCur.cx = bmMask.bmWidth;
					g_dimCur.cy = bmColor.bmHeight ? bmColor.bmHeight : bmMask.bmHeight / 2;
					g_hotCur.x = ii.xHotspot;
					g_hotCur.y = ii.yHotspot;
					if( FAILED( m_device->CreateTexture( g_dimCur.cx, g_dimCur.cy, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_texCur, NULL ) ) ||
						FAILED( m_device->CreateTexture( g_dimCur.cx, g_dimCur.cy, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_texCurSysMem, NULL ) ) )
					{
						logStr( 0, "WARNING: Failed to create mouse cursor textures. Mouse rendering disabled.\n" );
						mouseMode &= ~1;
					}
				}

				D3DLOCKED_RECT r;
				if( nullptr == m_texCur ||
					nullptr == m_texCurSysMem ||
					FAILED( m_texCurSysMem->LockRect( 0, &r, NULL, 0 ) ) ||
					0 == copyCursorBitmapToMappedTexture( ii.hbmMask, ii.hbmColor, bmMask, bmColor, r.pBits, r.Pitch )  ||
					FAILED( m_texCurSysMem->UnlockRect( 0 ) ) ||
					FAILED( m_device->UpdateTexture( m_texCurSysMem, m_texCur ) ) )
				{
					logStr( 0, "WARNING: Failed to fill mouse texture. Mouse rendering disabled.\n" );
					mouseMode &= ~1;
					SAFERELEASE( m_texCur );
					SAFERELEASE( m_texCurSysMem );
				}
				else
				{
					logStr( 3, "mouse cursor texture filled." );
				}
				::DeleteObject( ii.hbmColor );
				::DeleteObject( ii.hbmMask );
			}
		}
		else
		{
			logStr( 4, "no cursor set." );
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
		if( nullptr == m_texCur )
		{
			m_device->CreateTexture( 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_texCur, NULL );
		}
		float vals[4] = { -2.0f, -2.0f, 1.0f, 1.0f };
		m_device->SetPixelShaderConstantF( 7, vals, 1 );
	}
	SetTexture( 3, m_texCur );
	SetTexture( 4, m_texBlack );

	m_device->SetPixelShaderConstantF( 8, m_blackBias, 1 );

	// remove dependency of d3d9x.lib
	//D3DXMatrixOrthoOffCenterLH( &lOrthoMatrix, 0.0, static_cast<float>( m_sizeMap.cx ), 0.0, static_cast<float>( m_sizeMap.cy ), 0.0, 1.0 );
	//D3DXMATRIX* D3DXMatrixOrthoOffCenterLH(
	//	_Inout_ D3DXMATRIX* pOut,
	//	_In_    FLOAT      l,
	//	_In_    FLOAT      r,
	//	_In_    FLOAT      b,
	//	_In_    FLOAT      t,
	//	_In_    FLOAT      zn,
	//	_In_    FLOAT      zf
	//);
	// Set ortho projection filling the viewport
	D3DXMATRIX lOrthoMatrix = {
		 2.0f / m_sizeMap.cx,	 0.0f,					 0.0f,				 0.0f,
		 0.0f,					 2.0f / m_sizeMap.cy,	 0.0f,				 0.0f,
		 0.0f,					 0.0f,					 1.0f,				 0.0f,
		-1.0f,					-1.0f,					 0.0f,				 1.0f
	};
    res = m_device->SetTransform( D3DTS_PROJECTION, &lOrthoMatrix );

    // Set Identity view and world matrix
    D3DXMATRIX lViewMatrix;
    D3DXMatrixIdentity( &lViewMatrix );
    res = m_device->SetTransform( D3DTS_VIEW, &lViewMatrix ); 
    D3DXMATRIX lWorldMatrix;
    D3DXMatrixIdentity( &lWorldMatrix );
    res = m_device->SetTransform( D3DTS_WORLD, &lWorldMatrix);

    // Do the rendering to the current render target
    res = m_device->SetFVF( DXSCREENVERTEX::FVF );
    res = m_device->SetStreamSource( 0, m_VertexBuffer, 0, sizeof(DXSCREENVERTEX) );
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
		for( DWORD i = 0; i != ARRAYSIZE( ppOldTex ); i++ )
		{
			if( ppOldTex[i] )
			{
				m_device->SetTexture( i, ppOldTex[i] );
				ppOldTex[i]->Release();
			}
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

	logStr( 4, "Render end." );
	return VWB_ERROR_NONE;
}

//--------------------------------------------------------------------------
//
// Utility Functions
//

LPDIRECT3DVERTEXBUFFER9 DX9EXWarpBlend::CreateVertexBuffer(float width, float height) const
{
	LPDIRECT3DVERTEXBUFFER9 buf;

	if (SUCCEEDED(m_device->CreateVertexBuffer(sizeof(DXSCREENVERTEX) * 4, D3DUSAGE_WRITEONLY, DXSCREENVERTEX::FVF, D3DPOOL_DEFAULT, &buf, NULL)))
	{
		DXSCREENVERTEX *quad = NULL;
		if (SUCCEEDED(buf->Lock(0, 0, (void**)&quad, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK)))
		{

			quad[0].pos = D3DXVECTOR3(-0.5f, height + 0.5f, 0.0f);
			quad[1].pos = D3DXVECTOR3(width - 0.5f, height + 0.5f, 0.0f);
			quad[2].pos = D3DXVECTOR3(-0.5f, 0.5f, 0.0f);
			quad[3].pos = D3DXVECTOR3(width - 0.5f, 0.5f, 0.0f);
			quad[0].tex1 = D3DXVECTOR2(0.0f, 0.0f);
			quad[1].tex1 = D3DXVECTOR2(1.0f, 0.0f);
			quad[2].tex1 = D3DXVECTOR2(0.0f, 1.0f);
			quad[3].tex1 = D3DXVECTOR2(1.0f, 1.0f);

			buf->Unlock();
			return buf;
		}
		else
		{
			SAFERELEASE(buf);
		}
	}
	return NULL;
}

void DX9EXWarpBlend::SetTexture(unsigned int texId, LPDIRECT3DTEXTURE9 tex) const
{
    HRESULT res ;
    res = m_device->SetTexture( texId, tex ) ;
    res = m_device->SetTextureStageState(texId, D3DTSS_COLOROP, D3DTOP_SELECTARG1) ;
    res = m_device->SetTextureStageState(texId, D3DTSS_COLORARG1, D3DTA_TEXTURE) ;
    res = m_device->SetTextureStageState(texId, D3DTSS_COLORARG2, D3DTA_DIFFUSE) ;
    res = m_device->SetSamplerState(texId, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
    res = m_device->SetSamplerState(texId, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
    res = m_device->SetSamplerState(texId, D3DSAMP_BORDERCOLOR, D3DCOLOR_RGBA( 0, 0, 0, 0 ) );
    res = m_device->SetSamplerState(texId, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    res = m_device->SetSamplerState(texId, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    res = m_device->SetSamplerState(texId, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    res = m_device->SetSamplerState(texId, D3DSAMP_SRGBTEXTURE, 0 );
}
