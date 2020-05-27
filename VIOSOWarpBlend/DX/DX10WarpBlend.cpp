#include "DX10WarpBlend.h"
#include "pixelshader.h"

//#define XTDEBUG

#ifdef XTDEBUG
#include "../3rdparty/d3dX/Include/D3DX10.h"
#pragma comment( lib, "d3dx10.lib" )
#endif

//#pragma comment( lib, "d3d11.lib" )

const FLOAT _black[4] = {0,0,0,1};

DX10WarpBlend::DX10WarpBlend( ID3D10Device* pDevice )
:	DXWarpBlend(),
	m_device( pDevice ),
	m_VertexShader( NULL ),
	m_VertexBuffer( NULL ),
	//m_VertexBufferModel(NULL),
	//m_IndexBufferModel(NULL),
	m_PixelShader( NULL ),
	m_SSClamp( NULL ),
	m_SSLin( NULL ),
	m_ConstantBuffer( NULL ),
	m_texBlend( NULL ),
	m_texBlack( NULL ),
	m_texWarp( NULL ),
	m_texBB( NULL ),
	m_texWarpCalc( NULL ),
	m_RasterState( NULL ),
	m_Layout( NULL )
{
	if( NULL == m_device )
		throw( VWB_ERROR_PARAMETER );
	else
	{
		m_device->AddRef();
	}
	m_type4cc = '01XD';
}

DX10WarpBlend::~DX10WarpBlend(void)
{
	SAFERELEASE( m_Layout );
	SAFERELEASE( m_RasterState );
	SAFERELEASE( m_texWarpCalc );
	SAFERELEASE( m_texBB );
	SAFERELEASE( m_texWarp ); 
	SAFERELEASE( m_texBlend );
	SAFERELEASE( m_texBlack );
	SAFERELEASE( m_PixelShader );
	SAFERELEASE( m_VertexShader );
	SAFERELEASE( m_VertexBuffer );
	//SAFERELEASE( m_VertexBufferModel );
	//SAFERELEASE( m_IndexBufferModel );
	SAFERELEASE( m_SSLin );
	SAFERELEASE( m_SSClamp );
	SAFERELEASE( m_ConstantBuffer );
	SAFERELEASE( m_device );
	logStr( 1, "INFO: DX10-Warper destroyed.\n" );
}

VWB_ERROR DX10WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	HRESULT hr = E_FAIL;
	if( VWB_ERROR_NONE == err ) try
	{
		VWB_WarpBlend& wb = *wbs[calibIndex];

		ID3D10RenderTargetView* pVV = NULL;
		m_device->OMGetRenderTargets(1, &pVV, NULL );
		hr = E_FAIL;
		if( pVV )
		{
			ID3D10Resource* pRes = NULL;
			pVV->GetResource( &pRes );
			if( pRes )
			{
				ID3D10Texture2D* pTex;
				if( SUCCEEDED( pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (void**)&pTex ) ) )
				{
					D3D10_TEXTURE2D_DESC desc;
					pTex->GetDesc( &desc );
					pTex->Release();

					m_vp.Width = desc.Width;
					m_vp.Height = desc.Height;
					m_vp.TopLeftX = 0;
					m_vp.TopLeftY = 0;
					m_vp.MinDepth = 0.0f;
					m_vp.MaxDepth = 1.0f;
					logStr( 2, "INFO: Output buffer found. Viewport is %ux%u.\n", m_vp.Width, m_vp.Height );
					hr = S_OK;
				}
				pRes->Release();
			}
			pVV->Release();
		}
		if(FAILED(hr))
		{
			logStr( 0, "ERROR: Output buffer not found.\n" );
			return VWB_ERROR_GENERIC;
		}

		D3D10_TEXTURE2D_DESC descTexW = {
			(UINT)m_sizeMap.cx,//UINT Width;
			(UINT)m_sizeMap.cy,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			0 != ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R16G16_UNORM,//DXGI_FORMAT Format;
			{1,0},//DXGI_SAMPLE_DESC SampleDesc;
			D3D10_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D10_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0,//UINT MiscFlags;
		};
		D3D10_TEXTURE2D_DESC descTexB = {
			(UINT)m_sizeMap.cx,//UINT Width;
			(UINT)m_sizeMap.cy,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			DXGI_FORMAT_R16G16B16A16_UNORM,//DXGI_FORMAT Format;
			{1,0},//DXGI_SAMPLE_DESC SampleDesc;
			D3D10_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D10_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0,//UINT MiscFlags;
		};

		D3D10_TEXTURE2D_DESC descTexBl = {
			(UINT)m_sizeMap.cx,//UINT Width;
			(UINT)m_sizeMap.cy,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			DXGI_FORMAT_R8G8B8A8_UNORM,//DXGI_FORMAT Format;
			{1,0},//DXGI_SAMPLE_DESC SampleDesc;
			D3D10_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D10_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0,//UINT MiscFlags;
		};

		D3D10_SUBRESOURCE_DATA dataWarp;
		if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
		{
			UINT sz = 3 * m_sizeMap.cx;
			dataWarp.SysMemPitch = sizeof( float ) * sz;
			sz *= m_sizeMap.cy;
			dataWarp.SysMemSlicePitch = sizeof( float ) * sz;
			dataWarp.pSysMem = new float[sz];
			float* d = (float*)dataWarp.pSysMem;
			for( const VWB_WarpRecord *s = wb.pWarp, *sE = wb.pWarp + m_sizeMap.cx * m_sizeMap.cy; s != sE; d += 3, s++ )
			{
				d[0] = s->x;
				d[1] = s->y;
				d[2] = s->z;
			}
		}
		else
		{
			UINT sz = 2 * m_sizeMap.cx;
			dataWarp.SysMemPitch = sizeof( unsigned short ) * sz;
			sz *= m_sizeMap.cy;
			dataWarp.SysMemSlicePitch = sizeof( unsigned short ) * sz;
			dataWarp.pSysMem = new unsigned short[sz];
			unsigned short* d = (unsigned short*)dataWarp.pSysMem;
			for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 2, s++ )
			{
				d[0] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->x ) ) );
				d[1] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->y ) ) );
			}
		}

		D3D10_SUBRESOURCE_DATA dataBlend = {
			wb.pBlend2,
			sizeof( VWB_BlendRecord2 ) * m_sizeMap.cx,
			sizeof( VWB_BlendRecord2 ) * m_sizeMap.cx * m_sizeMap.cy
		};

		D3D10_SUBRESOURCE_DATA dataBlack = {
			wb.pBlack,
			sizeof( VWB_BlendRecord ) * m_sizeMap.cx,
			sizeof( VWB_BlendRecord ) * m_sizeMap.cx * m_sizeMap.cy
		};

		D3D10_SHADER_RESOURCE_VIEW_DESC descSRVW;
		descSRVW.Format = ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R16G16_UNORM;
		descSRVW.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		descSRVW.Texture2D.MipLevels = 1;
		descSRVW.Texture2D.MostDetailedMip = 0;
		D3D10_SHADER_RESOURCE_VIEW_DESC descSRVB = descSRVW;
		descSRVB.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		descSRVB.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		D3D10_SHADER_RESOURCE_VIEW_DESC descSRVBl = descSRVW;
		descSRVBl.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		descSRVBl.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		ID3D10Texture2D* pTexWarp = NULL, *pTexBlend = NULL, *pTexBlack = NULL;
		if(	FAILED( m_device->CreateTexture2D( &descTexW, &dataWarp, &pTexWarp ) ) ||
			FAILED( m_device->CreateTexture2D( &descTexB, &dataBlend, &pTexBlend ) ) ||
			FAILED( m_device->CreateTexture2D( &descTexB, dataBlack.pSysMem ? &dataBlack : nullptr, &pTexBlack ) ) ||
			FAILED( m_device->CreateShaderResourceView( pTexWarp, &descSRVW, &m_texWarp ) ) ||
			FAILED( m_device->CreateShaderResourceView( pTexBlend, &descSRVB, &m_texBlend ) ) ||
			FAILED( m_device->CreateShaderResourceView( pTexBlack, &descSRVBl, &m_texBlack ) ) 
			)
		{
			logStr( 0, "ERROR: Failed to create lookup textures.\n" );
			SAFERELEASE( pTexWarp );
			SAFERELEASE( pTexBlend );
			SAFERELEASE( pTexBlack );
			if( dataWarp.pSysMem )
			{
				if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
					delete[]( float* ) dataWarp.pSysMem;
				else
					delete[]( unsigned short* ) dataWarp.pSysMem;
			}
			throw VWB_ERROR_SHADER;
		}
		SAFERELEASE( pTexWarp );
		SAFERELEASE( pTexBlend );
		SAFERELEASE( pTexBlack );
		if( dataWarp.pSysMem )
		{
			if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
				delete[]( float* ) dataWarp.pSysMem;
			else
				delete[]( unsigned short* ) dataWarp.pSysMem;
		}

		ID3DBlob* pVSBlob = NULL;
		ID3DBlob* pErrBlob = NULL;
		hr = D3DCompile( s_pixelShaderDX4, sizeof(s_pixelShaderDX4), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: The vertex shader code cannot be compiled: %s\n", pErrBlob->GetBufferPointer() );
			SAFERELEASE( pErrBlob );
			throw VWB_ERROR_SHADER;
		}

		// Create the vertex shader0, 
		hr = m_device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_VertexShader );
		if( FAILED( hr ) )
		{	
			pVSBlob->Release();
			logStr( 0, "ERROR: The vertex shader cannot be created: %08X\n", hr );
			throw VWB_ERROR_SHADER;
		}
		FLOAT dx = 0.5f/m_sizeMap.cx;
		FLOAT dy = 0.5f/m_sizeMap.cy;
		SimpleVertex quad[] = { 
			{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
			{ {  1.0f + dx, -1.0f - dy, 0.5f }, { 1.0f, 1.0f } },
			{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },
			{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },
			{ { -1.0f - dx,  1.0f + dy, 0.5f }, { 0.0f, 0.0f } },
			{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
		};

		// Define the input layout
		D3D10_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( SimpleVertex, Pos ), D3D10_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( SimpleVertex, Tex ), D3D10_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE( layout );

		// Create the input layout
		hr = m_device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
												pVSBlob->GetBufferSize(), &m_Layout );
		pVSBlob->Release();
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create shader input layout: %08X\n", hr );
			throw VWB_ERROR_SHADER;
		}

		D3D10_BUFFER_DESC bd = {0};
		bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		bd.ByteWidth = sizeof(quad);
		D3D10_SUBRESOURCE_DATA data = { quad, 0, 0 };
		hr = m_device->CreateBuffer( &bd, &data, &m_VertexBuffer );
		
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create constant buffer: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		// Turn off culling, so we see the front and back of the triangle
		D3D10_RASTERIZER_DESC rasterDesc;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D10_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 1.0f;
		rasterDesc.DepthClipEnable = false;
		rasterDesc.FillMode = D3D10_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = true;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		hr = m_device->CreateRasterizerState( &rasterDesc, &m_RasterState );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create raster state: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		// Compile the pixel shader
        std::string pixelShader = "PS"; // or "TST"
#if 0
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
#endif
		ID3DBlob* pPSBlob = NULL;
		SAFERELEASE( pErrBlob );
		hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &pPSBlob, &pErrBlob );
//		hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &pPSBlob, &pErrBlob );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: The pixel shader code cannot be compiled (%08X): %s\n", hr, pErrBlob->GetBufferPointer() );
			SAFERELEASE( pErrBlob );
			throw VWB_ERROR_SHADER;
		}

		// Create the pixel shader
		hr = m_device->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), &m_PixelShader );
		pPSBlob->Release();
		if( FAILED( hr ) )
		{	
			logStr( 0, "ERROR: The pixel shader cannot be created: %08X\n", hr );
			throw VWB_ERROR_SHADER;
		}
		
		// Create the constant buffer
		bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
		bd.ByteWidth = sizeof(ConstantBuffer);
		hr = m_device->CreateBuffer( &bd, NULL, &m_ConstantBuffer );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not creare constant buffer: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		D3D10_SAMPLER_DESC descSam = {
			D3D10_FILTER_MIN_MAG_MIP_LINEAR, //D3D11_FILTER Filter;
			D3D10_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
			D3D10_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
			D3D10_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
			0, //FLOAT MipLODBias;
			1, //UINT MaxAnisotropy;
			D3D10_COMPARISON_NEVER, //D3D11_COMPARISON_FUNC ComparisonFunc;
			{0,0,0,0}, //FLOAT BorderColor[ 4 ];
			-FLT_MAX, //FLOAT MinLOD;
			FLT_MAX, //FLOAT MaxLOD;
		};
		m_device->CreateSamplerState( &descSam, &m_SSLin );
		descSam.AddressU = descSam.AddressV = descSam.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
		m_device->CreateSamplerState( &descSam, &m_SSClamp );

		logStr( 1, "SUCCESS: DX11-Warper initialized.\n" );

		// transpose view matrices
		m_mBaseI.Transpose();
		m_mViewIG.Transpose();
	} catch( VWB_ERROR e )
	{
		err = e;
	}

	return err;
}

VWB_ERROR DX10WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	HRESULT res = E_FAIL;
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT;

	if( NULL == m_PixelShader || NULL == m_device )
		return VWB_ERROR_GENERIC;

	ID3D10DepthStencilView* pDSV = NULL;
	ID3D10RenderTargetView* pRTV = NULL;
	m_device->OMGetRenderTargets( 1, &pRTV, &pDSV );

	// do backbuffer copy if necessary
	if( NULL == inputTexture ||
		VWB_UNDEFINED_GL_TEXTURE == inputTexture )
	{
		if( pRTV )
		{
			ID3D10Resource* pRes = NULL;
			pRTV->GetResource( &pRes );
			if( pRes )
			{
				ID3D10Texture2D* pTex;
				if( SUCCEEDED( pRes->QueryInterface( __uuidof( ID3D10Texture2D ), (void**)&pTex ) ) )
				{
					D3D10_TEXTURE2D_DESC desc;
					pTex->GetDesc( &desc );
					pTex->Release();
					if( NULL == m_texBB ||
						desc.Width != m_sizeIn.cx ||
						desc.Height != m_sizeIn.cy )
					{
						pTex = NULL;
						desc.Usage = D3D10_USAGE_DEFAULT;
						desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
						desc.CPUAccessFlags = 0;
						desc.ArraySize = 1;
						desc.MipLevels = 1;
						desc.MiscFlags = 0;
						res = m_device->CreateTexture2D( &desc, NULL, &pTex );
						m_sizeIn.cx = desc.Width;
						m_sizeIn.cy = desc.Height;
						if( SUCCEEDED( res ) )
						{
							D3D10_SHADER_RESOURCE_VIEW_DESC descSRV;
							descSRV.Format = desc.Format;
							descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
							descSRV.Texture2D.MipLevels= 1;
							descSRV.Texture2D.MostDetailedMip = 0;
							m_device->CreateShaderResourceView( (ID3D10Resource*)pTex, &descSRV, &m_texBB );
							pTex->Release();
						}
						else
							return VWB_ERROR_GENERIC;
					}
				}
				else
					return VWB_ERROR_GENERIC;

				if( NULL != m_texBB )
				{
					ID3D10Resource* pResDst = NULL;
					m_texBB->GetResource( &pResDst );
					m_device->CopyResource( pResDst, pRes );
					SAFERELEASE( pResDst );
				}
				pRes->Release();
			}
			else
				return VWB_ERROR_GENERIC;
		}
		else
			return VWB_ERROR_GENERIC;
	}
	else
	{
		ID3D10Texture2D* pResIn = (ID3D10Texture2D*)inputTexture;

		if( NULL != m_texBB )
		{
			ID3D10Resource* pRes = NULL;
			m_texBB->GetResource( &pRes );
			if( pRes != pResIn )
			{
				m_texBB->Release();
				m_texBB = NULL;
			}
			SAFERELEASE( pRes );
		}
		if( NULL == m_texBB )
		{
			D3D10_TEXTURE2D_DESC descTex;
			pResIn->GetDesc( &descTex );
			D3D10_SHADER_RESOURCE_VIEW_DESC desc;
			desc.Format = descTex.Format;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = 1;
			desc.Texture2D.MostDetailedMip = 0;

			res = m_device->CreateShaderResourceView( pResIn, &desc, &m_texBB );
            if (FAILED(res))
                return VWB_ERROR_GENERIC;

			m_sizeIn.cx = descTex.Width;
			m_sizeIn.cy = descTex.Height;
		}
	}

#ifdef XTDEBUG
	if( 0) 
	{
		ID3D10Resource* pRes = NULL;
		m_texBlend->GetResource( &pRes );
		D3DX10SaveTextureToFile( pRes, D3DX10_IFF_BMP, "D:\\Res_Blend.bmp" );
		SAFERELEASE( pRes )
		m_texWarp->GetResource( &pRes );
		D3DX10SaveTextureToFile( pRes, D3DX10_IFF_BMP, "D:\\Res_Warp.bmp" );
		SAFERELEASE( pRes )
		m_texBB->GetResource( &pRes );
		D3DX10SaveTextureToFile( pRes, D3DX10_IFF_BMP, "D:\\Res_BB.bmp" );
		SAFERELEASE( pRes )
	}
#endif

/////////////// save state
	ID3D10Buffer* pOldVtx = NULL;
	UINT oldStride = 0;
	UINT oldOffset = 0;
	ID3D10InputLayout* pOldLayout = NULL;
	D3D10_PRIMITIVE_TOPOLOGY oldTopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D10RasterizerState* pOldRS = NULL;

	ID3D10VertexShader* pOldVS = NULL;
	ID3D10Buffer* pOldCB = NULL;
	ID3D10PixelShader* pOldPS = NULL;
	ID3D10ShaderResourceView* ppOldSRV[5] = {0};
	ID3D10SamplerState* ppOldSS[5] = {0};

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		m_device->IAGetVertexBuffers( 0, 1, &pOldVtx, &oldStride, &oldOffset );
	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		m_device->IAGetInputLayout( &pOldLayout );
	if( VWB_STATEMASK_PRIMITIVE_TOPOLOGY & stateMask )
	    m_device->IAGetPrimitiveTopology( &oldTopo );
	if( VWB_STATEMASK_RASTERSTATE & stateMask )
		m_device->RSGetState( &pOldRS );

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		m_device->VSGetShader( &pOldVS );
	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		m_device->PSGetShader( &pOldPS );
	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
		m_device->PSGetShaderResources( 0, 5, ppOldSRV );
	if( VWB_STATEMASK_SAMPLER & stateMask )
		m_device->PSGetSamplers( 0, 5, ppOldSS );
	if( VWB_STATEMASK_CONSTANT_BUFFER & stateMask )
		m_device->PSGetConstantBuffers( 0, 1, &pOldCB );

////////////// set state
	UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;

	ConstantBuffer cb;
	memcpy( cb.matView, m_mVP.Transposed(), sizeof( cb.matView ) );
	cb.border[0] = m_bBorder;
	cb.border[1] = bDoNotBlend ? 0.0f : 1.0f;
	cb.border[2] = bDoNoBlack ? 0.0f : 1.0f;
	cb.params[0] = (FLOAT)m_sizeIn.cx;
	cb.params[1] = (FLOAT)m_sizeIn.cy;
	cb.params[2] = 1.0f/(FLOAT)m_sizeIn.cx;
	cb.params[3] = 1.0f/(FLOAT)m_sizeIn.cy;
	if( bPartialInput )
	{
		cb.offsScale[0] = (FLOAT)optimalRect.left / (FLOAT)optimalRes.cx;
		cb.offsScale[1] = (FLOAT)optimalRect.top / (FLOAT)optimalRes.cy;
		cb.offsScale[2] = (FLOAT)optimalRes.cx / ((FLOAT)optimalRect.right - (FLOAT)optimalRect.left );
		cb.offsScale[3] = (FLOAT)optimalRes.cy / ((FLOAT)optimalRect.bottom - (FLOAT)optimalRect.top );
	}
	else
	{
		cb.offsScale[0] = 0.0f;
		cb.offsScale[1] = 0.0f;
		cb.offsScale[2] = 1.0f;
		cb.offsScale[3] = 1.0f;
	}
	cb.blackBias[0] = m_blackBias.x;
	cb.blackBias[1] = m_blackBias.y;
	cb.blackBias[2] = m_blackBias.z;
	cb.blackBias[3] = 0;

	ID3D10ShaderResourceView* texCur = NULL;
	cb.offsScaleCur[0] = -2.0f;
	cb.offsScaleCur[1] = -2.0f;
	cb.offsScaleCur[2] = 1.0f;
	cb.offsScaleCur[3] = 1.0f;

	m_device->UpdateSubresource( m_ConstantBuffer, 0, NULL, &cb, 0, 0 );

	m_device->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );
	m_device->IASetInputLayout( m_Layout );
	m_device->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_device->VSSetShader( m_VertexShader );
	m_device->VSSetConstantBuffers( 0, 1, &m_ConstantBuffer );

	m_device->RSSetState( m_RasterState );
	m_device->PSSetShader( m_PixelShader );
	m_device->PSSetConstantBuffers( 0, 1, &m_ConstantBuffer );
	ID3D10ShaderResourceView* ppRes[] = { m_texBB ,m_texWarp, m_texBlend, nullptr, m_texBlack };
	ID3D10SamplerState* ppSam[] = { m_SSLin ,m_SSClamp, m_SSLin, m_SSLin, m_SSLin };
	m_device->PSSetShaderResources( 0, ARRAYSIZE(ppRes), ppRes );
	m_device->PSSetSamplers( 0, 5, ppSam );

////////////// draw
	if( VWB_STATEMASK_CLEARBACKBUFFER )
	{
		if( pDSV )
		{
			m_device->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0f, 0 );
			pDSV->Release();
		}
		if( pRTV )
		{
			m_device->ClearRenderTargetView( pRTV, _black );
			pRTV->Release();
		}
	}
	m_device->Draw( 6, 0 );
	res = S_OK;

/////////// restore state
	if( VWB_STATEMASK_CONSTANT_BUFFER & stateMask )
		m_device->PSSetConstantBuffers( 0, 1, &pOldCB );

	if( VWB_STATEMASK_SAMPLER & stateMask )
	{
		m_device->PSSetSamplers( 0, 5, ppOldSS );
		SAFERELEASE( ppOldSS[0] );
		SAFERELEASE( ppOldSS[1] );
		SAFERELEASE( ppOldSS[2] );
		SAFERELEASE( ppOldSS[3] );
		SAFERELEASE( ppOldSS[4] );
	}

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		m_device->PSSetShaderResources( 0, 5, ppOldSRV );
		SAFERELEASE( ppOldSRV[0] );
		SAFERELEASE( ppOldSRV[1] );
		SAFERELEASE( ppOldSRV[2] );
		SAFERELEASE( ppOldSRV[3] );
		SAFERELEASE( ppOldSRV[4] );
	}

	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		if( pOldPS )
		{
			m_device->PSSetShader( pOldPS );
			pOldPS->Release();
		}

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		if( pOldVS )
		{
			m_device->VSSetShader( pOldVS );
			pOldVS->Release();
		}

	if( VWB_STATEMASK_RASTERSTATE & stateMask )
		if( pOldRS )
		{
			m_device->RSSetState( pOldRS );
			pOldRS->Release();
		}

	if( VWB_STATEMASK_PRIMITIVE_TOPOLOGY & stateMask )
		m_device->IASetPrimitiveTopology( oldTopo );

	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		if( pOldLayout )
		{
			m_device->IASetInputLayout( pOldLayout );
			pOldLayout->Release();
		}

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		if( pOldVtx )
		{
			m_device->IASetVertexBuffers( 0, 1, &pOldVtx, &oldStride, &oldOffset );
			pOldVtx->Release();
		}

	return SUCCEEDED(res) ? VWB_ERROR_NONE : VWB_ERROR_GENERIC;
}