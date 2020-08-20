#ifndef VWB_WIN7_COMPAT
#include "DX12WarpBlend.h"

#include "pixelshader.h"
#include "atlbase.h"
#pragma comment( lib, "d3d12.lib" )

DX12WarpBlend::DX12WarpBlend( ID3D12CommandQueue* pCQ )
: DXWarpBlend()
, m_cq( NULL )
, m_device( NULL )
, m_ca( NULL )
, m_cl( NULL )
, m_srvHeap( NULL )
, m_rootSignature( NULL )
, m_pipelineState( NULL )
, m_vertexBuffer( NULL )
, m_vertexBufferView( { 0, 0, 0 } )
, m_texWarp( NULL )
, m_texBlend( NULL )
, m_texBlack( NULL )
, m_texCur( NULL )
, m_texBB( NULL )
, m_cb( NULL )
{
	if( NULL == pCQ )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( pCQ->QueryInterface( &m_cq ) ) )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( m_cq->GetDevice( IID_PPV_ARGS( &m_device  ) ) ) )
		throw( VWB_ERROR_PARAMETER );
		m_type4cc = '21XD';
}

DX12WarpBlend::~DX12WarpBlend(void)
{
	SAFERELEASE( m_texWarp );
	SAFERELEASE( m_texBlend );
	SAFERELEASE( m_texBlack );
	SAFERELEASE( m_texCur );
	SAFERELEASE( m_texBB );
	SAFERELEASE( m_cb );
	SAFERELEASE( m_vertexBuffer );
	SAFERELEASE( m_pipelineState );
	SAFERELEASE( m_rootSignature );
	SAFERELEASE( m_srvHeap );
	SAFERELEASE( m_device );
	SAFERELEASE( m_cl );
	logStr( 1, "INFO: DX11-Warper destroyed.\n" );
}

//------------------------------------------------------------------------------------------------
// Row-by-row memcpy
inline void MemcpySubresource(
	_In_ const D3D12_MEMCPY_DEST* pDest,
	_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
	SIZE_T RowSizeInBytes,
	UINT NumRows,
	UINT NumSlices )
{
	for( UINT z = 0; z < NumSlices; ++z )
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>( pDest->pData ) + pDest->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>( pSrc->pData ) + pSrc->SlicePitch * z;
		for( UINT y = 0; y < NumRows; ++y )
		{
			memcpy( pDestSlice + pDest->RowPitch * y,
					pSrcSlice + pSrc->RowPitch * y,
					RowSizeInBytes );
		}
	}
}

//------------------------------------------------------------------------------------------------
// All arrays must be populated (e.g. by calling GetCopyableFootprints)
inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	_In_range_( 0, D3D12_REQ_SUBRESOURCES ) UINT FirstSubresource,
	_In_range_( 0, D3D12_REQ_SUBRESOURCES - FirstSubresource ) UINT NumSubresources,
	UINT64 RequiredSize,
	_In_reads_( NumSubresources ) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	_In_reads_( NumSubresources ) const UINT* pNumRows,
	_In_reads_( NumSubresources ) const UINT64* pRowSizesInBytes,
	_In_reads_( NumSubresources ) const D3D12_SUBRESOURCE_DATA* pSrcData )
{
	// Minor validation
	auto IntermediateDesc = pIntermediate->GetDesc();
	auto DestinationDesc = pDestinationResource->GetDesc();
	if( IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
		RequiredSize > SIZE_T( -1 ) ||
		( DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
		  ( FirstSubresource != 0 || NumSubresources != 1 ) ) )
	{
		return 0;
	}

	BYTE* pData;
	HRESULT hr = pIntermediate->Map( 0, nullptr, reinterpret_cast<void**>( &pData ) );
	if( FAILED( hr ) )
	{
		return 0;
	}

	for( UINT i = 0; i < NumSubresources; ++i )
	{
		if( pRowSizesInBytes[i] > SIZE_T( -1 ) ) return 0;
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T( pLayouts[i].Footprint.RowPitch ) * SIZE_T( pNumRows[i] ) };
		MemcpySubresource( &DestData, &pSrcData[i], static_cast<SIZE_T>( pRowSizesInBytes[i] ), pNumRows[i], pLayouts[i].Footprint.Depth );
	}
	pIntermediate->Unmap( 0, nullptr );

	if( DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER )
	{
		pCmdList->CopyBufferRegion(
			pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width );
	}
	else
	{
		for( UINT i = 0; i < NumSubresources; ++i )
		{
			const D3D12_TEXTURE_COPY_LOCATION Dst = { pDestinationResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i + FirstSubresource };
			const D3D12_TEXTURE_COPY_LOCATION Src = { pIntermediate, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, pLayouts[i] };
			pCmdList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );
		}
	}
	return RequiredSize;
}

//------------------------------------------------------------------------------------------------
// Stack-allocating UpdateSubresources implementation
template <UINT MaxSubresources>
inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	UINT64 IntermediateOffset,
	_In_range_( 0, MaxSubresources ) UINT FirstSubresource,
	_In_range_( 1, MaxSubresources - FirstSubresource ) UINT NumSubresources,
	_In_reads_( NumSubresources ) D3D12_SUBRESOURCE_DATA* pSrcData )
{
	UINT64 RequiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
	UINT NumRows[MaxSubresources];
	UINT64 RowSizesInBytes[MaxSubresources];

	auto Desc = pDestinationResource->GetDesc();
	ID3D12Device* pDevice = nullptr;
	pDestinationResource->GetDevice( __uuidof( *pDevice ), reinterpret_cast<void**>( &pDevice ) );
	pDevice->GetCopyableFootprints( &Desc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts, NumRows, RowSizesInBytes, &RequiredSize );
	pDevice->Release();

	return UpdateSubresources( pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, Layouts, NumRows, RowSizesInBytes, pSrcData );
}

//------------------------------------------------------------------------------------------------
// Heap-allocating UpdateSubresources implementation
inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	UINT64 IntermediateOffset,
	_In_range_( 0, D3D12_REQ_SUBRESOURCES ) UINT FirstSubresource,
	_In_range_( 0, D3D12_REQ_SUBRESOURCES - FirstSubresource ) UINT NumSubresources,
	_In_reads_( NumSubresources ) D3D12_SUBRESOURCE_DATA* pSrcData )
{
	UINT64 RequiredSize = 0;
	UINT64 MemToAlloc = static_cast<UINT64>( sizeof( D3D12_PLACED_SUBRESOURCE_FOOTPRINT ) + sizeof( UINT ) + sizeof( UINT64 ) ) * NumSubresources;
	if( MemToAlloc > SIZE_MAX )
	{
		return 0;
	}
	void* pMem = HeapAlloc( GetProcessHeap(), 0, static_cast<SIZE_T>( MemToAlloc ) );
	if( pMem == nullptr )
	{
		return 0;
	}
	auto pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>( pMem );
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>( pLayouts + NumSubresources );
	UINT* pNumRows = reinterpret_cast<UINT*>( pRowSizesInBytes + NumSubresources );

	auto Desc = pDestinationResource->GetDesc();
	ID3D12Device* pDevice = nullptr;
	pDestinationResource->GetDevice( __uuidof( *pDevice ), reinterpret_cast<void**>( &pDevice ) );
	pDevice->GetCopyableFootprints( &Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize );
	pDevice->Release();

	UINT64 Result = UpdateSubresources( pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData );
	HeapFree( GetProcessHeap(), 0, pMem );
	return Result;
}

HRESULT CreateAndFillTexture( ID3D12Device* dev, ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_DESC const& desc, D3D12_SUBRESOURCE_DATA& data, ID3D12Resource*& tex, std::vector< CComPtr< ID3D12Resource > >& sts, LPCWSTR name = L"" )
{
	HRESULT hr = dev->CreateCommittedResource(
		&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS( &tex ) );

	UINT64 uploadBufferSize = 0;
	dev->GetCopyableFootprints( &desc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize );

	CComPtr<ID3D12Resource> uploadHeapTex;
	// Create the GPU upload buffer.
	hr = dev->CreateCommittedResource(
		&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
		D3D12_HEAP_FLAG_NONE,
		&D3D12_RESOURCE_DESC( { D3D12_RESOURCE_DIMENSION_BUFFER, 0, uploadBufferSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE } ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &uploadHeapTex ) );

	hr = uploadBufferSize == UpdateSubresources( cl, tex, uploadHeapTex, 0, 0, 1, &data ) ? S_OK : E_FAIL;
	if( SUCCEEDED( hr ) )
	{
		sts.push_back( uploadHeapTex );
		if( nullptr != name && 0 != name[0] )
			tex->SetName( name );
	}
	return hr;
}

VWB_ERROR DX12WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	HRESULT hr = E_FAIL;
	if( VWB_ERROR_NONE == err )
	{
		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 6;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if( FAILED( m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_srvHeap ) ) ) )
		{
			logStr( 0, "FATAL ERROR: Failed to create SRV/CBV descriptor heap.\n" );
			return VWB_ERROR_GENERIC;
		}
		m_srvHeap->SetName( L"VWB_srvheap" );

		heapDesc.NumDescriptors = 2;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if( FAILED( m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_samHeap ) ) ) )
		{
			logStr( 0, "FATAL ERROR: Failed to create sampler descriptor heap.\n" );
			return VWB_ERROR_GENERIC;
		}
		m_samHeap->SetName( L"VWB_samheap" );

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
					1, 0, 0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
					0
				},
				{
					D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					5, 0, 0,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
					D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
				}
			};

			D3D12_ROOT_PARAMETER1 rootParameters[] =
			{
				{
					D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ 1, &ranges[0] },
					D3D12_SHADER_VISIBILITY_ALL
				},
				{
					D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ 1, &ranges[1] },
					D3D12_SHADER_VISIBILITY_PIXEL
				}
			};

			D3D12_STATIC_SAMPLER_DESC samplers[] =
			{
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
				},
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

			};

			//// create sampler
			//{
			//	m_device->CreateSampler( )
			//}
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
				return VWB_ERROR_SHADER;
			}
			hr = m_device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "Error: CreateRootSignature failed with %x", hr );
				return VWB_ERROR_SHADER;
			}
			m_rootSignature->SetName( L"VWB_rootsignature" );
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
				return VWB_ERROR_SHADER;
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
				return VWB_ERROR_SHADER;
			}

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {
				m_rootSignature,													  // ID3D12RootSignature *pRootSignature;
				{ vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() },              // D3D12_SHADER_BYTECODE VS;
				{ psBlob->GetBufferPointer(), psBlob->GetBufferSize() },			  // D3D12_SHADER_BYTECODE PS;
				{ 0 }, { 0 }, { 0 },												  // D3D12_SHADER_BYTECODE DS;D3D12_SHADER_BYTECODE HS;D3D12_SHADER_BYTECODE GS;
				{0},																  // D3D12_STREAM_OUTPUT_DESC StreamOutput;
				{ FALSE, FALSE, {													  // D3D12_BLEND_DESC BlendState;
					TRUE,FALSE,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
					D3D12_LOGIC_OP_NOOP,
					D3D12_COLOR_WRITE_ENABLE_ALL } },
				UINT_MAX,															  //  UINT SampleMask;
				{	D3D12_FILL_MODE_SOLID,											  // D3D12_RASTERIZER_DESC RasterizerState;
					D3D12_CULL_MODE_NONE,
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
			hr = m_device->CreateGraphicsPipelineState( &psDesc, IID_PPV_ARGS( &m_pipelineState ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not create graphics pipeline: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			m_pipelineState->SetName( L"VWB_pipeline" );

			hr = m_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_ca ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not create command allocator: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			m_ca->SetName( L"VWB_commandallocator" );
			hr = m_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ca, m_pipelineState, IID_PPV_ARGS( &m_cl ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not create command list: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			m_cl->SetName( L"VWB_commandlist" );
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
					D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
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

			m_vertexBuffer->SetName( L"VWB_vertexbuffer" );
		}

		// Create the constant buffer
		{
			UINT sz = ( ( sizeof( ConstantBuffer ) + 255 ) / 256 ) * 256;
			hr = m_device->CreateCommittedResource(
				&D3D12_HEAP_PROPERTIES( {
					D3D12_HEAP_TYPE_UPLOAD,
					D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
					D3D12_MEMORY_POOL_UNKNOWN,
					1, 1 } ),
					D3D12_HEAP_FLAG_NONE,
					&D3D12_RESOURCE_DESC( {
						D3D12_RESOURCE_DIMENSION_BUFFER,
						0, sz, 1, 1, 1,
						DXGI_FORMAT_UNKNOWN,
						{1,0},
						D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
						D3D12_RESOURCE_FLAG_NONE } ),
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS( &m_cb )
						);
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
				m_cb->GetGPUVirtualAddress(),
				sz
			};
			m_device->CreateConstantBufferView(
				&desc,
				{ m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr }
			);
			m_cb->SetName( L"VWB_constantbuffer" );
		}

		// create and fill textures
		std::vector<CComPtr<ID3D12Resource>> stagingTexs;
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

			D3D12_SUBRESOURCE_DATA data;
			if( wb.header.flags & FLAG_WARPFILE_HEADER_3D )
			{
				UINT sz = 3 * m_sizeMap.cx;
				data.RowPitch = sizeof( float ) * sz;
				sz *= m_sizeMap.cy;
				data.SlicePitch = sizeof( float ) * sz;
				data.pData = new float[sz];
				float* d = (float*)data.pData;
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
				data.RowPitch = sizeof( unsigned short ) * sz;
				sz *= m_sizeMap.cy;
				data.SlicePitch = sizeof( unsigned short ) * sz;
				data.pData = new unsigned short[sz];
				unsigned short* d = (unsigned short*)data.pData;
				for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 2, s++ )
				{
					d[0] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->x ) ) );
					d[1] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->y ) ) );
				}
			}

			hr = CreateAndFillTexture( m_device, m_cl, textureDesc, data, m_texWarp, stagingTexs, L"VWB_warptexture" );
			delete[]( float* )data.pData;
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not fill warp texture: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}

			textureDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
			data.RowPitch = sizeof( unsigned short ) * 4 * m_sizeMap.cx;
			data.SlicePitch = data.RowPitch * m_sizeMap.cy;
			data.pData = wb.pBlend2;

			hr = CreateAndFillTexture( m_device, m_cl, textureDesc, data, m_texBlend, stagingTexs, L"VWB_blendtexture" );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not fill warp texture: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}

			// fill black texture
			char empty[16 * 16 * 4] = { 0 };
			if( wb.pBlack )
			{
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				data.RowPitch = sizeof( unsigned char ) * 4 * m_sizeMap.cx;
				data.SlicePitch = data.RowPitch * m_sizeMap.cy;
				data.pData = wb.pBlack;

				hr = CreateAndFillTexture( m_device, m_cl, textureDesc, data, m_texBlack, stagingTexs, L"VWB_blackleveltexture" );
				if( FAILED( hr ) )
				{
				logStr( 0, "ERROR: Could not fill black level texture: %08X\n", hr );
				return VWB_ERROR_SHADER;
				}
			}
			else
			{
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				textureDesc.Width = 16;
				textureDesc.Height = 16;
				data.RowPitch = sizeof( unsigned char ) * 4 * 16;
				data.SlicePitch = data.RowPitch * 16;
				data.pData = empty;

				hr = CreateAndFillTexture( m_device, m_cl, textureDesc, data, m_texBlack, stagingTexs, L"VWB_blackleveltexture" );
				if( FAILED( hr ) )
				{
					logStr( 0, "ERROR: Could not fill black level texture: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}
			}
			// prepare cursor texture
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = 16;
			textureDesc.Height = 16;
			data.RowPitch = sizeof( unsigned char ) * 4 * 16;
			data.SlicePitch = data.RowPitch * 16;
			data.pData = empty;

			hr = CreateAndFillTexture( m_device, m_cl, textureDesc, data, m_texCur, stagingTexs, L"VWB_cursortexture" );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not fill cursor texture: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}

			// Describe and create a SRVs for the texture.
			ID3D12Resource* res[]{ m_cb, m_texWarp, m_texBlend, m_texBlack, m_texCur, m_texBB };
			for( SIZE_T i = 1, s = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
				 i != ARRAYSIZE( res ); i++ )
			{
				if( nullptr != res[i] )
				{
					m_device->CreateShaderResourceView(
						res[i],
						NULL,  // use from texture
						{ m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + i * s }
					);
				}
			}

			D3D12_RESOURCE_BARRIER rBs[]{
				{
					D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{
						m_texWarp,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						D3D12_RESOURCE_STATE_COPY_DEST,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
					}
				},
				{
					D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{
						m_texBlend,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						D3D12_RESOURCE_STATE_COPY_DEST,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
					}
				},
				{
					D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{
						m_texBlack,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						D3D12_RESOURCE_STATE_COPY_DEST,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
					}
				},
				{
					D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAG_NONE,
					D3D12_RESOURCE_TRANSITION_BARRIER{
						m_texCur,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						D3D12_RESOURCE_STATE_COPY_DEST,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
					}
				},
			};
			m_cl->ResourceBarrier( ARRAYSIZE( rBs ), rBs );
		}

		// execute
		{
			hr = m_cl->Close();
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not fill command list: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			// Create synchronization objects and wait until assets have been uploaded to the GPU.
			CComPtr<ID3D12Fence> fence;
			hr = m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not create fence: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			hr = m_cq->Signal( fence, 0 );
			if( FAILED( hr ) )
			{
				logStr( 0, "ERROR: Could not set signal: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			UINT64 last = fence->GetCompletedValue();

			// Create an event handle to use for frame synchronization.
			HANDLE fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
			if( fenceEvent == nullptr )
			{
				logStr( 0, "ERROR: Could not create event: %08X\n", HRESULT_FROM_WIN32( GetLastError() ) );
				return VWB_ERROR_GENERIC;
			}

			// run list
			m_cq->ExecuteCommandLists( 1, (ID3D12CommandList*const*)&m_cl );

			hr = m_cq->Signal( fence, ++last );
			UINT64 current = fence->GetCompletedValue();
			if( current < last )
			{
				hr = fence->SetEventOnCompletion( last, fenceEvent );
				WaitForSingleObject( fenceEvent, INFINITE );
			}
		}

		logStr( 1, "SUCCESS: DX12-Warper initialized.\n" );

		// transpose view matrices
		m_mBaseI.Transpose();
		m_mViewIG.Transpose();
	}
	return err;
}

VWB_ERROR DX12WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	logStr( 5, "RenderDX12..." );

	HRESULT res = E_FAIL;
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT_D3D12;

	VWB_D3D12_RENDERINPUT* in = (VWB_D3D12_RENDERINPUT*)inputTexture;
	if( nullptr == in || nullptr == in->renderTarget || 0 == in->rtvHandlePtr )
		return VWB_ERROR_PARAMETER;
	HRESULT hr = S_OK;

	hr = m_ca->Reset();
	hr = m_cl->Reset( m_ca, m_pipelineState );

	ID3D12Resource* rt = (ID3D12Resource*)in->renderTarget;
	D3D12_RESOURCE_DESC desc = rt->GetDesc();

	if( nullptr == in->textureResource )
	{

		if( nullptr == m_texBB || desc.Width != m_texBB->GetDesc().Width )
		{
			// create tex#
			hr = m_device->CreateCommittedResource(
				&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				nullptr,
				IID_PPV_ARGS( &m_texBB ) );
			SIZE_T base = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			SIZE_T ds = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
			SIZE_T des = base + 5 * ds;
			m_device->CreateShaderResourceView(
				m_texBB,
				nullptr,
				{ des }
			);
			m_texBB->SetName( L"VWB_backbuffercopytexture" );
		}

		// prepare pipeline for copy
		D3D12_RESOURCE_BARRIER rB[]
		{
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				D3D12_RESOURCE_TRANSITION_BARRIER{
					(ID3D12Resource*)in->renderTarget,
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_COPY_SOURCE
				}
			},
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				D3D12_RESOURCE_TRANSITION_BARRIER{
					(ID3D12Resource*)m_texBB,
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COPY_DEST
				}
			},
		};
		m_cl->ResourceBarrier( ARRAYSIZE( rB ), rB );

		// issue copy
		m_cl->CopyResource( m_texBB, rt );

		// make rt render target again
		rB[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		rB[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rB[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		rB[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		m_cl->ResourceBarrier( ARRAYSIZE( rB ), rB );
	}
	else
	{
		if( m_texBB != (ID3D12Resource*)in->textureResource )
		{
			SAFERELEASE( m_texBB );
			SIZE_T base = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			SIZE_T ds = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
			SIZE_T des = base + 5 * ds;
			hr = in->textureResource->QueryInterface( &m_texBB );
			m_device->CreateShaderResourceView(
				m_texBB,
				nullptr,
				{ des }
			);
		}
		//D3D12_RESOURCE_BARRIER rB[]
		//{
		//	{
		//		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		//		D3D12_RESOURCE_BARRIER_FLAG_NONE,
		//		D3D12_RESOURCE_TRANSITION_BARRIER{
		//			(ID3D12Resource*)in->renderTarget,
		//			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		//			D3D12_RESOURCE_STATE_PRESENT,
		//			D3D12_RESOURCE_STATE_RENDER_TARGET
		//		}
		//	},
		//	//{
		//	//	D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		//	//	D3D12_RESOURCE_BARRIER_FLAG_NONE,
		//	//	D3D12_RESOURCE_TRANSITION_BARRIER{
		//	//		(ID3D12Resource*)m_texBB,
		//	//		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		//	//		D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		//	//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		//	//	}
		//	//},
		//};
		//m_cl->ResourceBarrier( ARRAYSIZE(rB), rB );
	}

	// copy constants
	{
		// Copy the triangle data to the vertex buffer.
		void* pData;
		ConstantBuffer cb;
		memcpy( cb.matView, m_mVP.Transposed(), sizeof( cb.matView ) );
		cb.border[0] = m_bBorder;
		cb.border[1] = bDoNotBlend ? 0.0f : 1.0f;
		cb.params[0] = (FLOAT)m_sizeIn.cx;
		cb.params[1] = (FLOAT)m_sizeIn.cy;
		cb.params[2] = 1.0f / (FLOAT)m_sizeIn.cx;
		cb.params[3] = 1.0f / (FLOAT)m_sizeIn.cy;
		if( bPartialInput )
		{
			cb.offsScale[0] = (FLOAT)optimalRect.left / (FLOAT)optimalRes.cx;
			cb.offsScale[1] = (FLOAT)optimalRect.top / (FLOAT)optimalRes.cy;
			cb.offsScale[2] = ( (FLOAT)optimalRect.right - (FLOAT)optimalRect.left ) / (FLOAT)optimalRes.cx;
			cb.offsScale[3] = ( (FLOAT)optimalRect.bottom - (FLOAT)optimalRect.top ) / (FLOAT)optimalRes.cy;
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

		D3D12_RANGE readRange = { 0, 0 };        // We do not intend to read from this resource on the CPU.
		hr = m_cb->Map( 0, &readRange, reinterpret_cast<void**>( &pData ) );
		memcpy( pData, &cb, sizeof( cb ) );
		m_cb->Unmap( 0, nullptr );
	}

	m_cl->SetGraphicsRootSignature( m_rootSignature );
	m_cl->SetDescriptorHeaps( 1, &m_srvHeap );
	m_cl->SetGraphicsRootDescriptorTable( 0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	const D3D12_VIEWPORT vp{ 0.0f, 0.0f, (FLOAT)desc.Width, (FLOAT)desc.Height, 0.0f, 1.0f };
	m_cl->RSSetViewports( 1, &vp );
	const D3D12_RECT sr{ 0, 0, (LONG)desc.Width, (LONG)desc.Height };
	m_cl->RSSetScissorRects( 1, &sr );
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]{ { in->rtvHandlePtr } };
	m_cl->OMSetRenderTargets( ARRAYSIZE( rtvs ), rtvs, FALSE, nullptr );
	if( VWB_STATEMASK_CLEARBACKBUFFER & stateMask )
	{
		if( in->rtvHandlePtr )
		{
			const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			m_cl->ClearRenderTargetView( { in->rtvHandlePtr }, clearColor, 0, nullptr );
		}
		else
		{
			logStr( 2, "WARNING: cannot clear render target if rtvHandlePtr is not set." );
		}
	}

	m_cl->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_cl->IASetVertexBuffers( 0, 1, &m_vertexBufferView );
	m_cl->DrawInstanced( 3, 2, 0, 0 );

	// flag rt for present
	D3D12_RESOURCE_BARRIER rB
	{
		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		D3D12_RESOURCE_BARRIER_FLAG_NONE,
		D3D12_RESOURCE_TRANSITION_BARRIER{
			(ID3D12Resource*)in->renderTarget,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		}
	};
	m_cl->ResourceBarrier( 1, &rB );

	hr = m_cl->Close();
	m_cq->ExecuteCommandLists( 1, (ID3D12CommandList*const*)&m_cl );

	return SUCCEEDED( res ) ? VWB_ERROR_NONE : VWB_ERROR_GENERIC;
}
#endif //ndef VWB_WIN7_COMPAT
