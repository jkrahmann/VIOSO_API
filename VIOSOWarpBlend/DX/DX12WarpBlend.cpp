#include "DX12WarpBlend.h"
#include "pixelshader.h"
#include "atlbase.h"
#pragma comment( lib, "d3d12.lib" )

DX12WarpBlend::DX12WarpBlend( ID3D12CommandList* pCQ )
: DXWarpBlend()
, m_cq( NULL )
, m_device( NULL )
, m_srvHeap( NULL )
, m_pipState( NULL )
, m_rootSignature( NULL )
{
	if( NULL == pCQ )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( pCQ->QueryInterface( &m_cq ) ) )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( m_cq->GetDevice( __uuidof( m_device ), (void**)&m_device ) ) )
		throw( VWB_ERROR_PARAMETER );
		m_type4cc = '21XD';
}

DX12WarpBlend::~DX12WarpBlend(void)
{
	SAFERELEASE( m_rootSignature );
	SAFERELEASE( m_srvHeap );
	SAFERELEASE( m_device );
	SAFERELEASE( m_cq );
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
			m_cq->
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


		}
		VWB_WarpBlend& wb = *wbs[calibIndex];


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