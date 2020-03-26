#include "DX11WarpBlend.h"
#include "pixelshader.h"

//#pragma comment( lib, "d3d11.lib" )

#pragma pack( push, 4 )
typedef struct ConstantBuffer
{
	FLOAT matView[16];
	FLOAT border[4];
	FLOAT params[4];
	FLOAT offsScale[4];
	FLOAT offsScaleCur[4];
} ConstantBuffer;
#pragma pack( pop )

const FLOAT _black[4] = {0,0,0,1};

bool SaveTex( LPCSTR path, ID3D11Device* dev, ID3D11DeviceContext* dc, ID3D11Texture2D* tex )
{
	bool ret = false;
	// create CPU accessable texture
	D3D11_TEXTURE2D_DESC descI = { 0 };
	D3D11_TEXTURE2D_DESC desc = { 0 };
	tex->GetDesc( &descI );
	logStr( 3, "Input texture dump desc:\n"
			"Width           = %i\n"
			"Height          = %i\n"
			"MipLevels       = %i\n"
			"ArraySize       = %i\n"
			"Format          = %i\n"
			"SampleDesc      = {Count = %i, Quality = %i}\n"
			"Usage           = %i\n"
			"BindFlags       = %i\n"
			"CPUAccessFlags  = %i\n"
			"MiscFlags       = %i\n"
			, descI.Width
			, descI.Height
			, descI.MipLevels
			, descI.ArraySize
			, descI.Format
			, descI.SampleDesc.Count
			, descI.SampleDesc.Quality
			, descI.Usage
			, descI.BindFlags
			, descI.CPUAccessFlags
			, descI.MiscFlags
	);


	desc.Format = descI.Format;
	desc.Width = descI.Width;
	desc.Height = descI.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	ID3D11Texture2D* pTexMem = NULL;
	HRESULT hr = dev->CreateTexture2D( &desc, NULL, &pTexMem );
	if( SUCCEEDED( hr ) )
	{
		dc->CopyResource( pTexMem, tex );
		D3D11_MAPPED_SUBRESOURCE res = { 0 };
		if( SUCCEEDED( dc->Map( pTexMem, 0, D3D11_MAP_READ, 0, &res ) ) )
		{
			if( DXGI_FORMAT_R8G8B8A8_UNORM == desc.Format ||
				DXGI_FORMAT_R8G8B8A8_TYPELESS == desc.Format ||
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB == desc.Format ||
				DXGI_FORMAT_R8G8B8A8_UINT == desc.Format ||
				DXGI_FORMAT_R8G8B8A8_SNORM == desc.Format ||
				DXGI_FORMAT_R8G8B8A8_SINT == desc.Format
				)
			{
				BITMAPINFOHEADER hdr = { 0 };
				hdr.biSize = sizeof( hdr );
				hdr.biWidth = desc.Width;
				hdr.biHeight = -LONG( desc.Height );
				hdr.biPlanes = 1;
				hdr.biBitCount = 32;
				hdr.biSizeImage = res.RowPitch * desc.Height;

				BITMAPFILEHEADER fh = { 0 };
				fh.bfType = 'MB';
				fh.bfOffBits = sizeof( fh ) + hdr.biSize;
				fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

				// swivel RGBA to BGRA
				unsigned char t = 0;
				for( unsigned char* px = (unsigned char*)res.pData, *pxE = ( (unsigned char*)res.pData ) + hdr.biSizeImage;
					 px != pxE; )
				{
					for( const unsigned char* pxLE = px + res.RowPitch; px != pxLE; px += 4 )
					{
						t = px[0];
						px[0] = px[2];
						px[2] = t;
					}
				}


				FILE* f = NULL;
				if( NO_ERROR == fopen_s( &f, path, "wb" ) )
				{
					fwrite( &fh, sizeof( fh ), 1, f );
					fwrite( &hdr, sizeof( hdr ), 1, f );
					fwrite( res.pData, hdr.biSizeImage, 1, f );
					fclose( f );
					ret = true;
					logStr( 3, "Texture dumped to %s", path );
				}
				else
				{
					logStr( 3, "Could not write file %s", path );
				}
			}
			else if( DXGI_FORMAT_R16G16B16A16_TYPELESS == desc.Format ||
					 DXGI_FORMAT_R16G16B16A16_UNORM == desc.Format ||
					 DXGI_FORMAT_R16G16B16A16_UINT == desc.Format ||
					 DXGI_FORMAT_R16G16B16A16_SNORM == desc.Format ||
					 DXGI_FORMAT_R16G16B16A16_SINT == desc.Format
				)
			{
				BITMAPINFOHEADER hdr = { 0 };
				hdr.biSize = sizeof( hdr );
				hdr.biWidth = desc.Width;
				hdr.biHeight = -LONG( desc.Height );
				hdr.biPlanes = 1;
				hdr.biBitCount = 32;
				hdr.biSizeImage = desc.Width * 4 * desc.Height;

				BITMAPFILEHEADER fh = { 0 };
				fh.bfType = 'MB';
				fh.bfOffBits = sizeof( fh ) + hdr.biSize;
				fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

				// swivel RGBA to BGRA
				unsigned char t = 0;
				unsigned char* pDst = new unsigned char[hdr.biSizeImage];
				unsigned char* pxD = pDst;
				for( UINT y = 0; y != desc.Height; y++ )
				{
					unsigned short* pxS = (unsigned short*)( ((unsigned char*)res.pData) + y * res.RowPitch );
					for( const unsigned short* pxLE = pxS + desc.Width * 4; pxS != pxLE; pxS += 4, pxD += 4 )
					{
						pxD[0] = unsigned char( pxS[1] >> 8 );
						pxD[1] = unsigned char( pxS[2] >> 8 );
						pxD[2] = unsigned char( pxS[0] >> 8 );
						pxD[3] = unsigned char( pxS[3] >> 8 );
					}
				}

				FILE* f = NULL;
				if( NO_ERROR == fopen_s( &f, path, "wb" ) )
				{
					fwrite( &fh, sizeof( fh ), 1, f );
					fwrite( &hdr, sizeof( hdr ), 1, f );
					fwrite( res.pData, hdr.biSizeImage, 1, f );
					fclose( f );
					ret = true;
					logStr( 3, "Texture dumped to %s", path );
				}
				else
				{
					logStr( 3, "Could not write file %s", path );
				}

				delete[] pDst;
			}
			else
			{
				logStr( 3, "Could not dump texture unknown format %i.", desc.Format );
			}
			dc->Unmap( pTexMem, 0 );
		}
		else
		{
			logStr( 3, "Could not map dump texture." );
		}
		SAFERELEASE( pTexMem );
	}
	else
	{
		logStr( 3, "Could not create dump texture (%08x).", hr );
	}
	return ret;
}

DX11WarpBlend::DX11WarpBlend( ID3D11Device* pDevice )
: DXWarpBlend(),
  m_device( pDevice ),
  m_dc( NULL ),
  m_VertexShader(NULL),
  m_VertexBuffer(NULL),
  m_VertexBufferModel(NULL),
  m_IndexBufferModel(NULL),
  m_PixelShader(NULL),
  m_SSClamp(NULL),
  m_SSLin(NULL),
  m_ConstantBuffer(NULL),
  m_texBlend(NULL),
  m_texWarp(NULL),
  m_texBB(NULL),
  m_texWarpCalc(NULL),
  m_texCur(NULL),
  m_focusWnd(0),
  m_RasterState(NULL),
  m_DepthState( NULL ),
  m_BlendState( NULL ),
  m_Layout(NULL)
{
	if( NULL == m_device )
		throw( VWB_ERROR_PARAMETER );
	else
	{
		m_device->AddRef();
		m_device->GetImmediateContext( &m_dc );
		if( NULL == m_dc )
			throw( VWB_ERROR_GENERIC );
	}
	m_type4cc = '11XD';
}

DX11WarpBlend::~DX11WarpBlend(void)
{
	SAFERELEASE( m_Layout );
	SAFERELEASE( m_DepthState );
	SAFERELEASE( m_BlendState );
	SAFERELEASE( m_RasterState );
	SAFERELEASE( m_texCur );
	SAFERELEASE( m_texWarpCalc );
	SAFERELEASE( m_texBB );
	SAFERELEASE( m_texWarp ); 
	SAFERELEASE( m_texBlend );
	SAFERELEASE( m_PixelShader );
	SAFERELEASE( m_VertexShader );
	SAFERELEASE( m_VertexBuffer );
	SAFERELEASE( m_VertexBufferModel );
	SAFERELEASE( m_IndexBufferModel );
	SAFERELEASE( m_SSLin );
	SAFERELEASE( m_SSClamp );
	SAFERELEASE( m_ConstantBuffer );
	SAFERELEASE( m_dc );
	SAFERELEASE( m_device );
	logStr( 1, "INFO: DX11-Warper destroyed.\n" );
}

VWB_ERROR DX11WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	m_focusWnd = ::GetActiveWindow();
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	HRESULT hr = E_FAIL;
	if( VWB_ERROR_NONE == err ) try
	{
		VWB_WarpBlend& wb = *wbs[calibIndex];

		ID3D11RenderTargetView* pVV = NULL;
		m_dc->OMGetRenderTargets(1, &pVV, NULL );
		hr = E_FAIL;
		if( pVV )
		{
			ID3D11Resource* pRes = NULL;
			pVV->GetResource( &pRes );
			if( pRes )
			{
				ID3D11Texture2D* pTex;
				if( SUCCEEDED( pRes->QueryInterface( __uuidof( ID3D11Texture2D ), (void**)&pTex ) ) )
				{
					D3D11_TEXTURE2D_DESC desc;
					pTex->GetDesc( &desc );
					pTex->Release();

					m_vp.Width = (FLOAT)desc.Width;
					m_vp.Height = (FLOAT)desc.Height;
					m_vp.TopLeftX = 0;
					m_vp.TopLeftY = 0;
					m_vp.MinDepth = 0.0f;
					m_vp.MaxDepth = 1.0f;
					logStr( 2, "INFO: Output buffer found. Viewport is %.0fx%.0f.\n", m_vp.Width, m_vp.Height );
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

		D3D11_TEXTURE2D_DESC descTexW = {
			(UINT)m_sizeMap.cx,//UINT Width;
			(UINT)m_sizeMap.cy,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			0 != ( wb.header.flags & FLAG_SP_WARPFILE_HEADER_3D ) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R16G16B16A16_UNORM,//DXGI_FORMAT Format;
			{1,0},//DXGI_SAMPLE_DESC SampleDesc;
			D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0,//UINT MiscFlags;
		};
		D3D11_TEXTURE2D_DESC descTexB = {
			(UINT)m_sizeMap.cx,//UINT Width;
			(UINT)m_sizeMap.cy,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			DXGI_FORMAT_R16G16B16A16_UNORM,//DXGI_FORMAT Format;
			{1,0},//DXGI_SAMPLE_DESC SampleDesc;
			D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0,//UINT MiscFlags;
		};

		D3D11_SUBRESOURCE_DATA dataWarp =
		{
			wb.pWarp,
			sizeof( VWB_WarpRecord ) * m_sizeMap.cx,
			sizeof( VWB_WarpRecord ) * m_sizeMap.cx * m_sizeMap.cy
		};
		if( 0 == ( wb.header.flags & FLAG_SP_WARPFILE_HEADER_3D ) )
		{
			UINT sz = 2 * m_sizeMap.cx;
			dataWarp.SysMemPitch = sizeof( float ) * sz;
			sz *= m_sizeMap.cy;
			dataWarp.SysMemSlicePitch = sizeof( float ) * sz;
			dataWarp.pSysMem = new float[sz];
			for( float* d = (float*)dataWarp.pSysMem, *s = (float*)wb.pWarp, *sE = ( (float*)wb.pWarp ) + m_sizeMap.cx * m_sizeMap.cy; s != sE; d += 2, s += 4 )
			{
				d[0] = s[0];
				d[1] = s[1];
			}
		} //TODO: fix texture sizes and types like GL
		else
		{
			UINT sz = 2 * m_sizeMap.cx;
			dataWarp.SysMemPitch = sizeof( float ) * sz;
			sz *= m_sizeMap.cy;
			dataWarp.SysMemSlicePitch = sizeof( float ) * sz;
			dataWarp.pSysMem = new float[sz];
			for( float* d = (float*)dataWarp.pSysMem, *s = (float*)wb.pWarp, *sE = ( (float*)wb.pWarp ) + m_sizeMap.cx * m_sizeMap.cy; s != sE; d += 2, s += 4 )
			{
				d[0] = s[0];
				d[1] = s[1];
			}
		}

		D3D11_SUBRESOURCE_DATA dataBlend = {
			wb.pBlend2,
			sizeof( VWB_BlendRecord2 ) * m_sizeMap.cx,
			sizeof( VWB_BlendRecord2 ) * m_sizeMap.cx * m_sizeMap.cy
		};

		D3D11_SHADER_RESOURCE_VIEW_DESC descSRVW;
		D3D11_SHADER_RESOURCE_VIEW_DESC descSRVB;
		descSRVW.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		descSRVW.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		descSRVW.Texture2D.MipLevels = 1;
		descSRVW.Texture2D.MostDetailedMip = 0;
		descSRVB = descSRVW;
		descSRVB.Format = DXGI_FORMAT_R16G16B16A16_UNORM;

		ID3D11Texture2D* pTexWarp = NULL, *pTexBlend = NULL;
		if(	FAILED( m_device->CreateTexture2D( &descTexW, &dataWarp, &pTexWarp ) ) ||
			FAILED( m_device->CreateTexture2D( &descTexB, &dataBlend, &pTexBlend ) ) ||
			FAILED( m_device->CreateShaderResourceView( pTexWarp, &descSRVW, &m_texWarp ) ) ||
			FAILED( m_device->CreateShaderResourceView( pTexBlend, &descSRVB, &m_texBlend ) ) )
		{
			if( wb.pWarp != dataWarp.pSysMem )
				delete[]( float* ) dataWarp.pSysMem;
			logStr( 0, "ERROR: Failed to create lookup textures.\n" );
			throw VWB_ERROR_SHADER;
		}
		SAFERELEASE( pTexWarp );
		SAFERELEASE( pTexBlend );
		if( wb.pWarp != dataWarp.pSysMem )
			delete[]( float* ) dataWarp.pSysMem;

		ID3DBlob* pVSBlob = NULL;
		ID3DBlob* pErrBlob = NULL;
		if( bFlipDXVs )
		{
			hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob );
		}
		else
		{
			hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob );
		}
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: The vertex shader code cannot be compiled: %s\n", pErrBlob->GetBufferPointer() );
			SAFERELEASE( pErrBlob );
			throw VWB_ERROR_SHADER;
		}

		// Create the vertex shader0, 
		hr = m_device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_VertexShader );
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
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( SimpleVertex, Pos ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( SimpleVertex, Tex ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

		D3D11_BUFFER_DESC bd = {0};
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.ByteWidth = sizeof(quad);
		D3D11_SUBRESOURCE_DATA data = { quad, 0, 0 };
		hr = m_device->CreateBuffer( &bd, &data, &m_VertexBuffer );
		
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create constant buffer: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		// Turn off culling, so we see the front and back of the triangle
		D3D11_RASTERIZER_DESC rasterDesc;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 1.0f;
		rasterDesc.DepthClipEnable = false;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
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

		D3D11_DEPTH_STENCIL_DESC dsdesc;
		memset( &dsdesc, 0, sizeof( dsdesc ) );
		dsdesc.DepthEnable = FALSE;
		dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsdesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		hr = m_device->CreateDepthStencilState( &dsdesc, &m_DepthState );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create depth state: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		D3D11_BLEND_DESC bdesc;
		memset( &bdesc, 0, sizeof( bdesc ) );
		bdesc.RenderTarget[0].BlendEnable = FALSE;
		bdesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
		hr = m_device->CreateBlendState( &bdesc, &m_BlendState );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not create blend state: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		// Compile the pixel shader
        std::string pixelShader = "PS"; // or "TST"
#if 1
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
		if( bFlipDXVs )
		{
			hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &pPSBlob, &pErrBlob );
		}
		else
		{
			hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &pPSBlob, &pErrBlob );
		}
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: The pixel shader code cannot be compiled (%08X): %s\n", hr, pErrBlob->GetBufferPointer() );
			SAFERELEASE( pErrBlob );
			throw VWB_ERROR_SHADER;
		}

		// Create the pixel shader
		hr = m_device->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_PixelShader );
		pPSBlob->Release();
		if( FAILED( hr ) )
		{	
			logStr( 0, "ERROR: The pixel shader cannot be created: %08X\n", hr );
			throw VWB_ERROR_SHADER;
		}
		
		// Create the constant buffer
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.ByteWidth = sizeof(ConstantBuffer);
		hr = m_device->CreateBuffer( &bd, NULL, &m_ConstantBuffer );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not creare constant buffer: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}

		D3D11_SAMPLER_DESC descSam = {
			D3D11_FILTER_MIN_MAG_MIP_LINEAR, //D3D11_FILTER Filter;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
			0, //FLOAT MipLODBias;
			1, //UINT MaxAnisotropy;
			D3D11_COMPARISON_NEVER, //D3D11_COMPARISON_FUNC ComparisonFunc;
			{0,0,0,0}, //FLOAT BorderColor[ 4 ];
			-FLT_MAX, //FLOAT MinLOD;
			FLT_MAX, //FLOAT MaxLOD;
		};
		m_device->CreateSamplerState( &descSam, &m_SSLin );
		descSam.AddressU = descSam.AddressV = descSam.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
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

VWB_ERROR DX11WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	HRESULT res = E_FAIL;
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT;

	logStr( 5, "RenderDX11..." );
	if( NULL == m_PixelShader || NULL == m_device )
	{
		logStr( 3, "DX device or shader program not available.\n" );
		return VWB_ERROR_GENERIC;
	}

	ID3D11DeviceContext* ctx = NULL;
	m_device->GetImmediateContext( &ctx );
	if( ctx != m_dc )
	{
		logStr( 3, "DX got other immediate context. Stop.\n" );
		SAFERELEASE( ctx );
		return VWB_ERROR_PARAMETER;
	}
	SAFERELEASE( ctx );

	ID3D11DepthStencilView* pDSV = NULL;
	ID3D11RenderTargetView* pRTV = NULL;
	if( NULL == inputTexture || ( stateMask & VWB_STATEMASK_CLEARBACKBUFFER ) )
	{
		m_dc->OMGetRenderTargets( 1, &pRTV, &pDSV );
		if( NULL == pRTV )
		{
			logStr( 3, "Render target not available.\n" );
			return VWB_ERROR_GENERIC;
		}
	}

	// do backbuffer copy if necessary
	if( NULL == inputTexture ||
		VWB_UNDEFINED_GL_TEXTURE == inputTexture )
	{
		ID3D11Resource* pRes = NULL;
		pRTV->GetResource( &pRes );
		if( pRes )
		{
			ID3D11Texture2D* pTex;
			if( SUCCEEDED( pRes->QueryInterface( &pTex ) ) )
			{
				if( 3 < g_logLevel )
				{
					char path[MAX_PATH];
					strcpy_s( path, g_logFilePath );
					strcat_s( path, ".bb.bmp" );
					SaveTex( path, m_device, m_dc, pTex );
				}
				D3D11_TEXTURE2D_DESC desc;
				pTex->GetDesc( &desc );
				pTex->Release();
				if( NULL == m_texBB ||
					desc.Width != m_sizeIn.cx ||
					desc.Height != m_sizeIn.cy )
				{
					logStr( 1, "new backbuffer texture resource attached." );
					logStr( 3, "Backbuffer texture desc:\n"
							"Width           = %i\n"
							"Height          = %i\n"
							"MipLevels       = %i\n"
							"ArraySize       = %i\n"
							"Format          = %i\n"
							"SampleDesc      = {Count = %i, Quality = %i}\n"
							"Usage           = %i\n"
							"BindFlags       = %i\n"
							"CPUAccessFlags  = %i\n"
							"MiscFlags       = %i\n"
							, desc.Width
							, desc.Height
							, desc.MipLevels
							, desc.ArraySize
							, desc.Format
							, desc.SampleDesc.Count
							, desc.SampleDesc.Quality
							, desc.Usage
							, desc.BindFlags
							, desc.CPUAccessFlags
							, desc.MiscFlags
					);

					pTex = NULL;
					//desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					desc.CPUAccessFlags = 0;
					desc.ArraySize = 1;
					desc.MipLevels = 1;
					desc.MiscFlags = 0;

					logStr( 3, "Clone texture desc:\n"
							"Width           = %i\n"
							"Height          = %i\n"
							"MipLevels       = %i\n"
							"ArraySize       = %i\n"
							"Format          = %i\n"
							"SampleDesc      = {Count = %i, Quality = %i}\n"
							"Usage           = %i\n"
							"BindFlags       = %i\n"
							"CPUAccessFlags  = %i\n"
							"MiscFlags       = %i\n"
							, desc.Width
							, desc.Height
							, desc.MipLevels
							, desc.ArraySize
							, desc.Format
							, desc.SampleDesc.Count
							, desc.SampleDesc.Quality
							, desc.Usage
							, desc.BindFlags
							, desc.CPUAccessFlags
							, desc.MiscFlags
					);

					res = m_device->CreateTexture2D( &desc, NULL, &pTex );
					m_sizeIn.cx = desc.Width;
					m_sizeIn.cy = desc.Height;
					m_vp.Width = (FLOAT)desc.Width;
					m_vp.Height = (FLOAT)desc.Height;
					if( SUCCEEDED( res ) )
					{
						D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
						if( DXGI_FORMAT_R16G16B16A16_TYPELESS == desc.Format )
							desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
						else if( DXGI_FORMAT_R8G8B8A8_TYPELESS == desc.Format )
							desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						descSRV.Format = desc.Format;
						descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
						descSRV.Texture2D.MipLevels= 1;
						descSRV.Texture2D.MostDetailedMip = 0;
						res = m_device->CreateShaderResourceView( (ID3D11Resource*)pTex, &descSRV, &m_texBB );
						pTex->Release();
						if( SUCCEEDED( res ) )
						{
							logStr( 2, "Backbuffer cloned." );
						}
						else
						{
							logStr( 2, "Failed to create clone texture view (%0x08x).\n", res );
							SAFERELEASE( m_texBB );
							return VWB_ERROR_GENERIC;
						}
					}
					else
					{
						logStr( 2, "Failed to create clone texture from current render target.\n" );
						return VWB_ERROR_GENERIC;
					}
				}
				else
				{
					logStr( 3, "Backbuffer clone texture acquired; format: %i \n", desc.Format );
				}
			}
			else
			{
				logStr( 2, "Failed to get attached texture from render target; cannot copy.\n" );
				return VWB_ERROR_GENERIC;
			}

			if( NULL != m_texBB )
			{
				logStr( 4, "Copy backbuffer to input texture..." );
				ID3D11Resource* pResDst = NULL;
				m_texBB->GetResource( &pResDst );
				m_dc->CopyResource( pResDst, pRes );
				SAFERELEASE( pResDst );
				logStr( 4, " Done.\n" );
			}
			else
			{
				logStr( 3, "Error: Missing backbuffer resource to copy." );
			}
			pRes->Release();
		}
		else
		{
			logStr( 2, "Failed to get attached resource from render target; cannot copy.\n" );
			return VWB_ERROR_GENERIC;
		}
	}
	else
	{
		ID3D11Texture2D* pTexIn = NULL;
		if( FAILED( ((IUnknown *)inputTexture)->QueryInterface( &pTexIn ) ) )
		{
			logStr( 2, "Failed to query ID3D11Texture2D input texture.\n" );
			return VWB_ERROR_PARAMETER;
		}

		if( NULL != m_texBB )
		{
			ID3D11Resource* pRes = NULL;
			m_texBB->GetResource( &pRes );
			if( pRes != pTexIn )
			{
				m_texBB->Release();
				m_texBB = NULL;
			}
			SAFERELEASE( pRes );
		}
		if( NULL == m_texBB )
		{
			D3D11_TEXTURE2D_DESC descTex;
			pTexIn->GetDesc( &descTex );
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			if( DXGI_FORMAT_R8G8B8A8_TYPELESS == descTex.Format )
			{
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			else if( DXGI_FORMAT_R16G16B16A16_TYPELESS == descTex.Format )
			{
				desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			}
			else if( DXGI_FORMAT_R32G32B32A32_TYPELESS == descTex.Format )
			{
				desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else
			{
				desc.Format = descTex.Format;
			}

			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = 1;
			desc.Texture2D.MostDetailedMip = 0;

			res = m_device->CreateShaderResourceView( pTexIn, &desc, &m_texBB );
			pTexIn->Release();
			if( FAILED( res ) )
			{
				logStr( 2, "Failed to create resource view from input texture.\n" );
				return VWB_ERROR_GENERIC;
			}

			m_sizeIn.cx = descTex.Width;
			m_sizeIn.cy = descTex.Height;
			m_vp.Width = (FLOAT)descTex.Width;
			m_vp.Height = (FLOAT)descTex.Height;
			logStr( 2, "Input texture resource attached." );
			logStr( 3, "Input texture desc:\n"
                    "Width           = %i\n"
                    "Height          = %i\n"
                    "MipLevels       = %i\n"
                    "ArraySize       = %i\n"
                    "Format          = %i\n"
                    "SampleDesc      = {Count = %i, Quality = %i}\n"
                    "Usage           = %i\n"
                    "BindFlags       = %i\n"
                    "CPUAccessFlags  = %i\n"
                    "MiscFlags       = %i\n"
                    , descTex.Width
					, descTex.Height
					, descTex.MipLevels
					, descTex.ArraySize
					, descTex.Format
					, descTex.SampleDesc.Count
					, descTex.SampleDesc.Quality
					, descTex.Usage
					, descTex.BindFlags
					, descTex.CPUAccessFlags
					, descTex.MiscFlags
			);
		}
		SAFERELEASE( pTexIn );
	}

	if( 3 < g_logLevel )
	{
		ID3D11Resource* pRes = NULL;
		m_texBB->GetResource( &pRes );

		if( NULL != pRes )
		{
			ID3D11Texture2D* pTex = NULL;
			if( SUCCEEDED( pRes->QueryInterface( &pTex ) ) )
			{
				char path[MAX_PATH];
				strcpy_s( path, g_logFilePath );
				strcat_s( path, ".texin.bmp" );
				SaveTex( path, m_device, m_dc, pTex );
				SAFERELEASE( pTex );
			}
			else
			{
				logStr( 3, "Could not query input texture." );
			}
			SAFERELEASE( pRes );
		}
		else
		{
			logStr( 3, "Could not get input resource." );
		}
	}
	logStr( 5, "RenderDX11, input tex set." );

/////////////// save state
	ID3D11Buffer* pOldVtx = NULL;
	UINT oldStride = 0;
	UINT oldOffset = 0;
	ID3D11InputLayout* pOldLayout = NULL;
	D3D11_PRIMITIVE_TOPOLOGY oldTopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ID3D11RasterizerState* pOldRS = NULL;
	ID3D11BlendState* pOldBS = NULL;
	ID3D11DepthStencilState* pOldDS = NULL;

	ID3D11VertexShader* pOldVS = NULL;
	ID3D11Buffer* pOldCBVS = NULL;
	ID3D11Buffer* pOldCBPS = NULL;
	ID3D11PixelShader* pOldPS = NULL;
	ID3D11ShaderResourceView* ppOldSRV[4] = {0};
	FLOAT bf[4] = { 1,1,1,1 };
	UINT dsm = 0xFFFFFFFF;

	ID3D11SamplerState* ppOldSS[4] = {0};

	UINT oldNVP = 0;
	D3D11_VIEWPORT pOldVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { 0 };

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		m_dc->IAGetVertexBuffers( 0, 1, &pOldVtx, &oldStride, &oldOffset );
	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		m_dc->IAGetInputLayout( &pOldLayout );
	if( VWB_STATEMASK_PRIMITIVE_TOPOLOGY & stateMask )
	    m_dc->IAGetPrimitiveTopology( &oldTopo );
	if( VWB_STATEMASK_RASTERSTATE & stateMask )
	{
		m_dc->RSGetViewports( &oldNVP, NULL );
		m_dc->RSGetViewports( &oldNVP, pOldVP );
		m_dc->RSGetState( &pOldRS );
		m_dc->OMGetBlendState( &pOldBS, bf, &dsm );
		m_dc->OMGetDepthStencilState( &pOldDS, 0 );
	}

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		m_dc->VSGetShader( &pOldVS, NULL, NULL );
	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		m_dc->PSGetShader( &pOldPS, NULL, NULL );
	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
		m_dc->PSGetShaderResources( 0, 4, ppOldSRV );
	if( VWB_STATEMASK_SAMPLER & stateMask )
		m_dc->PSGetSamplers( 0, 4, ppOldSS );
	if( VWB_STATEMASK_CONSTANT_BUFFER & stateMask )
	{
		m_dc->VSGetConstantBuffers( 0, 1, &pOldCBVS );
		m_dc->PSGetConstantBuffers( 0, 1, &pOldCBPS );
	}

////////////// set state
	UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;

	ConstantBuffer cb;
	memcpy( cb.matView, m_mVP.Transposed(), sizeof( cb.matView ) );
	cb.border[0] = m_bBorder;
	cb.border[1] = bDoNotBlend ? 0.0f : 1.0f;
	cb.params[0] = (FLOAT)m_sizeIn.cx;
	cb.params[1] = (FLOAT)m_sizeIn.cy;
	cb.params[2] = 1.0f/(FLOAT)m_sizeIn.cx;
	cb.params[3] = 1.0f/(FLOAT)m_sizeIn.cy;
	if( bPartialInput )
	{
		cb.offsScale[0] = (FLOAT)optimalRect.left / (FLOAT)optimalRes.cx;
		cb.offsScale[1] = (FLOAT)optimalRect.top / (FLOAT)optimalRes.cy;
		cb.offsScale[2] = ((FLOAT)optimalRect.right - (FLOAT)optimalRect.left ) / (FLOAT)optimalRes.cx;
		cb.offsScale[3] = ((FLOAT)optimalRect.bottom - (FLOAT)optimalRect.top ) / (FLOAT)optimalRes.cy;
	}
	else
	{
		cb.offsScale[0] = 0.0f;
		cb.offsScale[1] = 0.0f;
		cb.offsScale[2] = 1.0f;
		cb.offsScale[3] = 1.0f;
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
					D3D11_TEXTURE2D_DESC descTex = {
						(UINT)g_dimCur.cx,//UINT Width;
						(UINT)g_dimCur.cy,//UINT Height;
						1,//UINT MipLevels;
						1,//UINT ArraySize;
						DXGI_FORMAT_B8G8R8A8_UNORM,//DXGI_FORMAT Format;
						{1,0},//DXGI_SAMPLE_DESC SampleDesc;
						D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
						D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
						0,//UINT CPUAccessFlags;
						0,//UINT MiscFlags;
					};
					D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
					descSRV.Format = descTex.Format;
					descSRV.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
					descSRV.Texture2D.MipLevels = 1;
					descSRV.Texture2D.MostDetailedMip = 0;
					ID3D11Texture2D* pTexCur = NULL;
					if( FAILED( m_device->CreateTexture2D( &descTex, NULL, &pTexCur ) ) ||
						FAILED( m_device->CreateShaderResourceView( pTexCur, &descSRV, &m_texCur ) ) )
					{
						logStr( 0, "WARNING: Failed to create mouse cursor textures. Mouse rendering disabled.\n" );
						mouseMode &= ~1;
					}
					SAFERELEASE( pTexCur );
				}

				BYTE* pData = new BYTE[4 * g_dimCur.cx  * g_dimCur.cy];
				ID3D11Resource* res;
				if( nullptr != m_texCur ||
					0 != copyCursorBitmapToMappedTexture( ii.hbmMask, ii.hbmColor, bmMask, bmColor, pData, 4 * g_dimCur.cx ) )
				{
					m_texCur->GetResource( &res );
					m_dc->UpdateSubresource( res, 0, NULL, pData, 4 * g_dimCur.cx, 4 * g_dimCur.cx * g_dimCur.cy );
				}
				else
				{
					logStr( 0, "WARNING: Failed to fill mouse texture. Mouse rendering disabled.\n" );
					mouseMode &= ~1;
					SAFERELEASE( m_texCur );
				}
				::DeleteObject( ii.hbmColor );
				::DeleteObject( ii.hbmMask );
				SAFERELEASE( res );
				delete[] pData;

			}
		}

		RECT rWnd = { 0 };
		if( GetWindowRect( m_focusWnd, &rWnd ) )
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

			cb.offsScaleCur[0] = static_cast<float>( ci.ptScreenPos.x - g_hotCur.x * 4 / 3 - rWnd.left ) / static_cast<float>( rWnd.right - rWnd.left );
			cb.offsScaleCur[1] = static_cast<float>( ci.ptScreenPos.y - g_hotCur.y * 4 / 3 - rWnd.top ) / static_cast<float>( rWnd.bottom - rWnd.top );
			cb.offsScaleCur[2] = static_cast<float>( rWnd.right - rWnd.left ) / static_cast<float>( g_dimCur.cx * 4 / 3 );
			cb.offsScaleCur[3] = static_cast<float>( rWnd.bottom - rWnd.top ) / static_cast<float>( g_dimCur.cx * 4 / 3 );
		}
		else
		{
			cb.offsScaleCur[0] = -2.0f;
			cb.offsScaleCur[1] = -2.0f;
			cb.offsScaleCur[2] = 1.0f;
			cb.offsScaleCur[3] = 1.0f;
		}
	}
	else
	{
		cb.offsScaleCur[0] = -2.0f;
		cb.offsScaleCur[1] = -2.0f;
		cb.offsScaleCur[2] = 1.0f;
		cb.offsScaleCur[3] = 1.0f;
	}

	m_dc->UpdateSubresource( m_ConstantBuffer, 0, NULL, &cb, 0, 0 );

	m_dc->IASetVertexBuffers( 0, 1, &m_VertexBuffer, &stride, &offset );
	m_dc->IASetInputLayout( m_Layout );
	m_dc->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_dc->VSSetShader( m_VertexShader, NULL, 0 );
	m_dc->VSSetConstantBuffers( 0, 1, &m_ConstantBuffer );

	m_dc->RSSetState( m_RasterState );
	//m_dc->RSSetViewports( 1, &m_vp );
	m_dc->PSSetShader( m_PixelShader, NULL, 0 );
	m_dc->PSSetConstantBuffers( 0, 1, &m_ConstantBuffer );
	m_dc->PSSetShaderResources( 0, 1, &m_texBB );
	m_dc->PSSetShaderResources( 1, 1, &m_texWarp );
	m_dc->PSSetShaderResources( 2, 1, &m_texBlend );
	m_dc->PSSetShaderResources( 3, 1, &m_texCur );
	m_dc->PSSetSamplers( 0, 1, &m_SSLin );
	m_dc->PSSetSamplers( 1, 1, &m_SSClamp );

	m_dc->OMSetBlendState( m_BlendState, NULL, 0xFFFFFFFF );
	m_dc->OMSetDepthStencilState( m_DepthState, 0 );

	if( pDSV )
	{
		////////////// clear
		if( stateMask & VWB_STATEMASK_CLEARBACKBUFFER )
		{
			m_dc->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		}
		pDSV->Release();
	}
	if( pRTV )
	{
		////////////// clear
		if( stateMask & VWB_STATEMASK_CLEARBACKBUFFER )
		{
			m_dc->ClearRenderTargetView( pRTV, _black );
		}
		pRTV->Release();
	}

	////////////// draw
	m_dc->Draw( 6, 0 );
	res = S_OK;

/////////// restore state
	if( VWB_STATEMASK_CONSTANT_BUFFER & stateMask )
	{
		m_dc->VSSetConstantBuffers( 0, 1, &pOldCBVS );
		SAFERELEASE( pOldCBVS );
		m_dc->PSSetConstantBuffers( 0, 1, &pOldCBPS );
		SAFERELEASE( pOldCBPS );
	}

	if( VWB_STATEMASK_SAMPLER & stateMask )
	{
		m_dc->PSSetSamplers( 0, 4, ppOldSS );
		SAFERELEASE( ppOldSS[0] );
		SAFERELEASE( ppOldSS[1] );
		SAFERELEASE( ppOldSS[2] );
		SAFERELEASE( ppOldSS[3] );
	}

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		m_dc->PSSetShaderResources( 0, 4, ppOldSRV );
		SAFERELEASE( ppOldSRV[0] );
		SAFERELEASE( ppOldSRV[1] );
		SAFERELEASE( ppOldSRV[2] );
		SAFERELEASE( ppOldSRV[3] );
	}

	if( VWB_STATEMASK_PIXEL_SHADER & stateMask )
		if( pOldPS )
		{
			m_dc->PSSetShader( pOldPS, NULL, NULL );
			pOldPS->Release();
		}

	if( VWB_STATEMASK_VERTEX_SHADER & stateMask )
		if( pOldVS )
		{
			m_dc->VSSetShader( pOldVS, NULL, NULL );
			pOldVS->Release();
		}

	if( VWB_STATEMASK_RASTERSTATE & stateMask )
	{
		if( pOldRS )
		{
			m_dc->RSSetState( pOldRS );
			pOldRS->Release();
		}

		if( pOldBS )
		{
			m_dc->OMSetBlendState( pOldBS, bf, dsm );
			pOldBS->Release();
		}

		if( pOldDS )
		{
			m_dc->OMSetDepthStencilState( pOldDS, 0 );
			pOldDS->Release();
		}

		if( 0 != oldNVP && 0 != pOldVP[0].Width )
		{
			m_dc->RSSetViewports( oldNVP, pOldVP );
		}
	}

	if( VWB_STATEMASK_PRIMITIVE_TOPOLOGY & stateMask )
		m_dc->IASetPrimitiveTopology( oldTopo );

	if( VWB_STATEMASK_INPUT_LAYOUT & stateMask )
		if( pOldLayout )
		{
			m_dc->IASetInputLayout( pOldLayout );
			pOldLayout->Release();
		}

	if( VWB_STATEMASK_VERTEX_BUFFER & stateMask )
		if( pOldVtx )
		{
			m_dc->IASetVertexBuffers( 0, 1, &pOldVtx, &oldStride, &oldOffset );
			pOldVtx->Release();
		}

	return SUCCEEDED(res) ? VWB_ERROR_NONE : VWB_ERROR_GENERIC;
}