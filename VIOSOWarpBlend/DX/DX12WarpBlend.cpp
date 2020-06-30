#ifndef VWB_WIN7_COMPAT
#include "DX12WarpBlend.h"
#endif //ndef VWB_WIN7_COMPAT

#include "pixelshader.h"
#include "atlbase.h"
#pragma comment( lib, "d3d12.lib" )

DX12WarpBlend::DX12WarpBlend( ID3D12GraphicsCommandList* pCL )
: DXWarpBlend()
, m_cl( NULL )
, m_device( NULL )
, m_srvHeap( NULL )
, m_rootSignature( NULL )
, m_pipelineState( NULL )
{
	if( NULL == pCL )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( pCL->QueryInterface( &m_cl ) ) )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( m_cl->GetDevice( __uuidof( m_device ), (void**)&m_device ) ) )
		throw( VWB_ERROR_PARAMETER );
		m_type4cc = '21XD';
}

DX12WarpBlend::~DX12WarpBlend(void)
{
	SAFERELEASE( m_rootSignature );
	SAFERELEASE( m_srvHeap );
	SAFERELEASE( m_device );
	SAFERELEASE( m_cl );
	logStr( 1, "INFO: DX11-Warper destroyed.\n" );
}

VWB_ERROR DX12WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	HRESULT hr = E_FAIL;
	if( VWB_ERROR_NONE == err )
	{
		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 5;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if( FAILED( m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_srvHeap ) ) ) )
		{
			logStr( 0, "FATAL ERROR: Failed to create SRV/CBV descriptor heap.\n" );
			return VWB_ERROR_GENERIC;
		}

		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE sig = { D3D_ROOT_SIGNATURE_VERSION_1 };

			if( FAILED( m_device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &sig, sizeof( sig ) ) ) )
			{
				logStr( 0, "FATAL ERROR: Failed to create root signature.\n" );
				return VWB_ERROR_GENERIC;
			}

			D3D12_DESCRIPTOR_RANGE1 ranges[] =
			{
				{
					D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
					1, 1, 0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
					D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
				},
				{
					D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					5, 1, 0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
					D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
				}
			};

			D3D12_ROOT_PARAMETER1 rootParameters[] =
			{
				{
					D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ _countof( ranges ), ranges },
					D3D12_SHADER_VISIBILITY_ALL
				}
			};

			D3D12_STATIC_SAMPLER_DESC samplers[] =
			{
				{
					D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					0,
					0,
					D3D12_COMPARISON_FUNC_NEVER,
					D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
					0.0f,
					D3D12_FLOAT32_MAX,
					0,
					0,
					D3D12_SHADER_VISIBILITY_PIXEL
				},
				{
					D3D12_FILTER_MIN_MAG_MIP_LINEAR,
					D3D12_TEXTURE_ADDRESS_MODE_BORDER,
					D3D12_TEXTURE_ADDRESS_MODE_BORDER,
					D3D12_TEXTURE_ADDRESS_MODE_BORDER,
					0,
					0,
					D3D12_COMPARISON_FUNC_NEVER,
					D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
					0.0f,
					D3D12_FLOAT32_MAX,
					1,
					0,
					D3D12_SHADER_VISIBILITY_PIXEL
				}
			};

			D3D12_ROOT_SIGNATURE_DESC1 rsd =
			{
				_countof( rootParameters ),
				rootParameters,
				_countof( samplers ),
				samplers,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			};
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			rootSignatureDesc.Desc_1_1 = rsd;
			CComPtr< ID3DBlob > signature;
			CComPtr< ID3DBlob > error;

			if( FAILED( D3D12SerializeVersionedRootSignature( &rootSignatureDesc, &signature, &error ) ) )
			{
				logStr( 0, "Error: Serializing root signature: %s", nullptr != error ? (char*)error->GetBufferPointer() : "unkown" );
				throw VWB_ERROR_SHADER;
			}
			hr = m_device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "Error: CreateRootSignature failed with %x", hr );
				throw VWB_ERROR_SHADER;
			}
		}

		{
			// gather viewport information
			ID3D12Resource* pRV = NULL;
			/*
			m_cl->O
			//m_dc->OMGetRenderTargets( 1, &pVV, NULL );
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
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Output buffer not found.\n" );
				return VWB_ERROR_GENERIC;
			}

*/
		}

		// build pipeline
		{
			// Define the input layout
			D3D12_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( SimpleVertex, Pos ), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( SimpleVertex, Tex ), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
			// compile shader
			CComPtr< ID3DBlob > errBlob;
			CComPtr< ID3DBlob > vsBlob;
			CComPtr< ID3DBlob > psBlob;
			if( bFlipDXVs )
			{
				hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, &errBlob );
			}
			else
			{
				hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, &errBlob );
			}
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: The vertex shader code cannot be compiled: %s\n", errBlob->GetBufferPointer() );
				throw VWB_ERROR_SHADER;
			}

			std::string pixelShader = "PS"; // or "TST"
			#if 1
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
			if( bFlipDXVs )
			{
				hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &psBlob, &errBlob );
			}
			else
			{
				hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, pixelShader.c_str(), "ps_4_0", 0, 0, &psBlob, &errBlob );
			}
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: The pixel shader code cannot be compiled (%08X): %s\n", hr, errBlob->GetBufferPointer() );
				throw VWB_ERROR_SHADER;
			}

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
				m_rootSignature,													  // ID3D12RootSignature *pRootSignature;
				{ vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() },              // D3D12_SHADER_BYTECODE VS;
				{ psBlob->GetBufferPointer(), psBlob->GetBufferSize() },			  // D3D12_SHADER_BYTECODE PS;
				{ 0 }, { 0 }, { 0 },												  // D3D12_SHADER_BYTECODE DS;D3D12_SHADER_BYTECODE HS;D3D12_SHADER_BYTECODE GS;
				{0},																  // D3D12_STREAM_OUTPUT_DESC StreamOutput;
				{ FALSE, FALSE, {													  // D3D12_BLEND_DESC BlendState;
					FALSE,FALSE,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_LOGIC_OP_NOOP,
					D3D12_COLOR_WRITE_ENABLE_ALL } },
				UINT_MAX,															  //  UINT SampleMask;
				{	D3D12_FILL_MODE_SOLID,											  // D3D12_RASTERIZER_DESC RasterizerState;
					D3D12_CULL_MODE_BACK,
					FALSE,
					D3D12_DEFAULT_DEPTH_BIAS,
					D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
					D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
					TRUE,
					FALSE,
					FALSE,
					0,
					D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF },
				{ 0 },																  // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
				{ layout, _countof( layout ) },										  // D3D12_INPUT_LAYOUT_DESC InputLayout;
				D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,						  // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
				D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,								  // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
				1,																	  // UINT NumRenderTargets;
				{DXGI_FORMAT_R8G8B8A8_UNORM},										  // DXGI_FORMAT RTVFormats[ 8 ];
				DXGI_FORMAT_UNKNOWN,												  // DXGI_FORMAT DSVFormat;
				{ 1, DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 },						  // DXGI_SAMPLE_DESC SampleDesc;
				0,																	  // UINT NodeMask; 
				{NULL, 0},															  // D3D12_CACHED_PIPELINE_STATE CachedPSO;
				D3D12_PIPELINE_STATE_FLAG_NONE										  // D3D12_PIPELINE_STATE_FLAGS Flags;
			};
			hr = m_device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_pipelineState ) );

			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not create graphics pipeline: %08X\n", hr );
				throw VWB_ERROR_SHADER;
			}
		}
		/*
		D3D11_BUFFER_DESC bd = { 0 };
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.ByteWidth = sizeof( quad );
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
		#endif
		ID3DBlob* pPSBlob = NULL;
		SAFERELEASE( pErrBlob );

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
		bd.ByteWidth = sizeof( ConstantBuffer );
		hr = m_device->CreateBuffer( &bd, NULL, &m_ConstantBuffer );
		if( FAILED( hr ) )
		{
			logStr( 0, "ERROR: Could not creare constant buffer: %08X\n", hr );
			throw VWB_ERROR_GENERIC;
		}



		// compile a shader set

		VWB_WarpBlend& wb = *wbs[calibIndex];

		*/
	}
	return err;
}

VWB_ERROR DX12WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	HRESULT res = E_FAIL;
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT;

	logStr( 5, "RenderDX12..." );
	//if( NULL == m_PixelShader || NULL == m_device )
	//{
	//	logStr( 3, "DX device or shader program not available.\n" );
	//	return VWB_ERROR_GENERIC;
	//}
	return SUCCEEDED( res ) ? VWB_ERROR_NONE : VWB_ERROR_GENERIC;
}
#endif //ndef VWB_WIN7_COMPAT
