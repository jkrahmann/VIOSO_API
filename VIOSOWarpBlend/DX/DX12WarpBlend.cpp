#ifndef VWB_WIN7_COMPAT
#include "DX12WarpBlend.h"

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
, m_vertexBuffer( NULL )
, m_vertexBufferView( { 0, 0, 0 } )
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
	SAFERELEASE( m_vertexBuffer );
	SAFERELEASE( m_pipelineState );
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
		// create and fill vertex buffer
		{
			FLOAT dx = 0.5f / m_sizeMap.cx;
			FLOAT dy = 0.5f / m_sizeMap.cy;
			SimpleVertex quad[] = {
				{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
				{ {  1.0f + dx, -1.0f - dy, 0.5f }, { 1.0f, 1.0f } },
				{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },
				{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },
				{ { -1.0f - dx,  1.0f + dy, 0.5f }, { 0.0f, 0.0f } },
				{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
			};
			hr = m_device->CreateCommittedResource(
				&D3D12_HEAP_PROPERTIES( { 
					D3D12_HEAP_TYPE_UPLOAD, 
					D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
					D3D12_MEMORY_POOL_UNKNOWN,
					1, 1 } ),
				D3D12_HEAP_FLAG_NONE,
				&D3D12_RESOURCE_DESC( {
					D3D12_RESOURCE_DIMENSION_BUFFER,
					0, sizeof( quad ), 1, 1, 1,
					DXGI_FORMAT_UNKNOWN,
					{1,0},
					D3D12_TEXTURE_LAYOUT_UNKNOWN,
					D3D12_RESOURCE_FLAG_NONE }),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &m_vertexBuffer )
			);
			// Copy the triangle data to the vertex buffer.
			UINT8* pVertexDataBegin;
			D3D12_RANGE readRange = { 0, 0 };        // We do not intend to read from this resource on the CPU.
			hr = m_vertexBuffer->Map( 0, &readRange, reinterpret_cast<void**>( &pVertexDataBegin ) );
			memcpy( pVertexDataBegin, quad, sizeof( quad ) );
			m_vertexBuffer->Unmap( 0, nullptr );

			// Initialize the vertex buffer view.
			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof( SimpleVertex );
			m_vertexBufferView.SizeInBytes = sizeof( quad );
		}

		// create and fill textures
		{
			VWB_WarpBlend& wb = *wbs[calibIndex];

			// the warp texture
			// it is RGBA32, we need RG16U in case of 2D and RGB32F in case of 3D
			D3D12_RESOURCE_DESC textureDesc = {
				D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				0, //UINT64 Alignment;
				(UINT64)m_sizeMap.cx,//UINT Width;
				(UINT64)m_sizeMap.cy,//UINT Height;
				1,//UINT16 DepthOrArraySize;
				1,//UINT16 MipLevels;
				0 != ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R16G16_UNORM,//DXGI_FORMAT Format;
				{1,0},//DXGI_SAMPLE_DESC SampleDesc;
				D3D12_TEXTURE_LAYOUT_UNKNOWN,// D3D12_TEXTURE_LAYOUT Layout;
				D3D12_RESOURCE_FLAG_NONE,// D3D12_RESOURCE_FLAGS Flags;
			};

			hr = m_device->CreateCommittedResource(
				&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS( &m_texWarp ) );

			UINT64 uploadBufferSize = 0;
			m_device->GetCopyableFootprints( &textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize );
			CComPtr<ID3D12Resource> uploadHeapTex;

			// Create the GPU upload buffer.
			hr = m_device->CreateCommittedResource(
				&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
				D3D12_HEAP_FLAG_NONE,
				&D3D12_RESOURCE_DESC( { D3D12_RESOURCE_DIMENSION_BUFFER, 0, uploadBufferSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE } ),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS( &uploadHeapTex ) );

			D3D12_SUBRESOURCE_DATA dataWarp;
			if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
			{
				UINT sz = 3 * m_sizeMap.cx;
				dataWarp.RowPitch = sizeof( float ) * sz;
				sz *= m_sizeMap.cy;
				dataWarp.SlicePitch = sizeof( float ) * sz;
				dataWarp.pData = new float[sz];
				float* d = (float*)dataWarp.pData;
				for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 3, s++ )
				{
					d[0] = s->x;
					d[1] = s->y;
					d[2] = s->z;
				}
			}
			else
			{
				UINT sz = 2 * m_sizeMap.cx;
				dataWarp.RowPitch = sizeof( unsigned short ) * sz;
				sz *= m_sizeMap.cy;
				dataWarp.SlicePitch = sizeof( unsigned short ) * sz;
				dataWarp.pData = new unsigned short[sz];
				unsigned short* d = (unsigned short*)dataWarp.pData;
				for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 2, s++ )
				{
					d[0] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->x ) ) );
					d[1] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->y ) ) );
				}
			}


			//UpdateSubresources( m_cl, m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData );
			//m_commandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );

			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			m_device->CreateShaderResourceView( m_texWarp, &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart() );

		}

		// Create the constant buffer
		{
		}
		/*
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
