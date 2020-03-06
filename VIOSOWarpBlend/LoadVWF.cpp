#include "Platform.h"

#include "LoadVWF.h"
#include "logging.h"
#include "PathHelper.h"
#include "Platform.h"

#include <stdlib.h>

bool DeleteVWF( VWB_WarpBlend& wb )
{
		if( wb.pWarp ) 
			delete[] wb.pWarp;
		if( wb.pBlend )
			delete[] wb.pBlend;
		if( wb.pBlack ) 
			delete[] wb.pBlack;
		if( wb.pWhite ) 
			delete[] wb.pWhite;
		wb.header.width = 0;
		wb.header.height = 0;
	return true;
}

bool DeleteVWF( VWB_WarpBlendSet& set )
{
	for( VWB_WarpBlendSet::iterator it = set.begin(); it != set.end(); it = set.erase( it ) )
	{
		if( NULL != *it )
		{
			DeleteVWF(**it);
			delete (*it);
		}
	}
	return true;
}

bool DeleteVWF( VWB_WarpBlendHeaderSet& set )
{
	for( VWB_WarpBlendHeaderSet::iterator it = set.begin(); it != set.end(); it = set.erase( it ) )
	{
		if( NULL != *it )
		{
			delete (*it);
		}
	}
	return true;
}

VWB_ERROR LoadVWF( VWB_WarpBlendSet& set, char const* path )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: LoadVWF: Missing path to warp file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	VWB_ERROR ret = VWB_ERROR_NONE;
	char const* pP = path;
	char pp[MAX_PATH];
	char const* pP2;
	do
	{
		VWB_WarpBlendSet::size_type nBefore = set.size();
		pP2 = strchr( pP, ',' );
		if( pP2 )
			strncpy_s( pp, pP, pP2 - pP );
		else
			strcpy( pp, pP );
		MkPath( pp, MAX_PATH, ".vwf" );

		logStr( 2, "Open \"%s\"...\n", pp );
		FILE* f = NULL;
		errno_t err = NO_ERROR;
		if( NO_ERROR == ( err = fopen_s( &f, pp, "rb" ) ) )
		{
			logStr( 2, "File found and openend.\n" );
			VWB_WarpSetFileHeader h0;
			int nSets = 1;
			while( nSets )
			{
				long s = ftell( f );
				if( 1 == fread_s( &h0, sizeof( VWB_WarpSetFileHeader ), sizeof( VWB_WarpSetFileHeader ), 1, f ) )
				{
					switch( *(VWB_uint*)&h0.magicNumber )
					{
					case '3fwv':
					case '2fwv':
					case '0fwv':
						{
							logStr( 2, "Load warp map %d...\n", VWB_uint(set.size()) );
							VWB_WarpBlend* pWB = new VWB_WarpBlend;
							memset( pWB, 0, sizeof( VWB_WarpBlend ) );
							memcpy( pWB, &h0, sizeof( h0 ) );
							// read remainder of header
							size_t sh = sizeof( pWB->header );
							if( sh > pWB->header.szHdr )
								sh = pWB->header.szHdr;
							size_t s1 = sizeof( h0 );
							size_t s2 = sizeof( pWB->header );
							if( sh > s1 && 1 == fread_s( ((char*)&pWB->header) + s1, s2 - s1, sh - s1, 1, f ) )
							{
								fseek( f, (long)pWB->header.szHdr - (long)sh, SEEK_CUR );
								int nRecords = pWB->header.width * pWB->header.height;
								if( 0 != nRecords && 0 != pWB->header.hMonitor )
								{
									strcpy( pWB->path, pp );
									pWB->pWarp = new VWB_WarpRecord[nRecords];
									if( nRecords == fread_s( pWB->pWarp, nRecords * sizeof( VWB_WarpRecord ), sizeof( VWB_WarpRecord ), nRecords, f ) )
									{
										set.push_back( pWB );

										logStr( 2, "Warp map successfully loaded, new dataset (%d) %dx%d created.\n", VWB_uint(set.size()), pWB->header.width, pWB->header.height );
										nSets--;
										break;
									}
									else
										logStr( 0, "ERROR: Malformed file data.\n" );
								}
								else
									logStr( 0, "ERROR: Malformed file header.\n" );
							}
							else
								logStr( 0, "ERROR: Unexpected end of file.\n" );
							ret = VWB_ERROR_VWF_LOAD;
							nSets = 0;
							delete pWB;
							break;
						}
						break;
					case '1fwv':
						// load a set
						nSets = h0.numBlocks;
						fseek( f, h0.offs, SEEK_SET );
						logStr( 2, "File contains %d chunks.\n", nSets );
						break;
					default:
						if( 'B' == h0.magicNumber[0] && 'M' == h0.magicNumber[1] )
						{ // load bitmap
							if( 0 != set.size() )
							{
								logStr( 2, "Load bitmap for dataset %d...\n", VWB_uint(set.size()) );
								BITMAPFILEHEADER& bmfh = *(BITMAPFILEHEADER*)&h0; // as BITMAPFILEHEADER is smaller than vwf header
								// bmfh.bfOffBits: offset of actual data
								// fill BITMAPINFOHEADER:
								BITMAPINFOHEADER bmih;
								// fill first bytes with last bytes of vwf file header
								VWB_uint s1 = sizeof( h0 );
								VWB_uint s2 = sizeof(bmfh);
								VWB_uint s3 = sizeof(bmih);
								memcpy( (VWB_byte*)&bmih, ((VWB_byte*)&h0)+s2, s1 - s2 );
								// read remaining from file
								if( 1 == fread_s( 
									((VWB_byte*)&bmih) + s1 - s2,
									s3 - s1 + s2,
									s3 - s1 + s2, 1, f ) )
								{
									if( bmfh.bfOffBits != s2 + s3 )
										fseek( f, (long)bmfh.bfOffBits - (long)s2 - (long)s3, SEEK_CUR );
									if( 24 == bmih.biBitCount ||
										32 == bmih.biBitCount )
									{
										bool bTopDown = 0 > bmih.biHeight;
										int w = bmih.biWidth;
										int h = abs( bmih.biHeight );
										if( h == (VWB_int)set.back()->header.height &&
											w == (VWB_int)set.back()->header.width )
										{
											int n = w * h;
											int bmStep = bmih.biBitCount / 8;
											int bmPitch = (( w * bmih.biBitCount + 31 ) / 32 ) * 4;
											int bmPadding =  bmPitch - w * bmStep;
											int bmSize = bmPitch * h;
											VWB_BlendRecord* pB = new VWB_BlendRecord[ n ];
											char* pBMData = new char[ bmSize ];
											if( h == (int)fread_s( pBMData, bmSize, bmPitch, h, f ) )
											{

												if( bTopDown )
												{
													if( 32 == bmih.biBitCount )
														memcpy( pB, pBMData, n * sizeof( VWB_BlendRecord ) ); // trivial, just copy the whole buffer
													else
													{
														char* pBML = pBMData;
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pBML+= bmPadding ) 
															for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML+= 3 )
															{
																pL->r = pBML[2];
																pL->g = pBML[1];
																pL->b = pBML[0];
																pL->a = 255;
															}
													}
												}
												else
												{
													if( 32 == bmih.biBitCount )
													{ // reverse order
														char* pBML = pBMData + bmPitch * ( h - 1 );
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL+= w, pBML-= bmPitch ) // padding is always 0
															memcpy( pL, pBML, n * sizeof( VWB_BlendRecord ) );
													}
													else
													{
														char* pBML = pBMData + bmPitch * ( h - 1 );
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL+= w, pBML-= bmPitch - bmPadding )
															for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML+= 3 )
															{ // todo: interpret bitfield mask here
																pL->r = pBML[2];
																pL->g = pBML[1];
																pL->b = pBML[0];
																pL->a = 255;
															}
													}
												}

												if( set.back()->pBlend )
												{
													if( set.back()->pBlack )
													{
														if( set.back()->pWhite )
														{
															delete[] pBMData;
															logStr( 0, "WARNING: Too many image maps. Ignoring!\n" );
														}
														else
														{
															set.back()->pWhite = pB;
															logStr( 2, "White image set.\n" );
														}

													}
													else
													{
														set.back()->pBlack = pB;
														logStr( 2, "Black image set.\n" );
													}
												}
												else
												{
													set.back()->pBlend = pB;
													logStr( 2, "Blend image set.\n" );
												}
												nSets--;
												if( bmSize + bmfh.bfOffBits != bmfh.bfSize )
												{
													fseek( f, bmfh.bfSize - bmfh.bfOffBits - bmSize, SEEK_CUR );
												}
												break;
											}
											else 
											{
												if( pBMData )
													delete[] pBMData;
												logStr( 0, "ERROR: Unexpected end of file. Bitmap or vwf file broken!\n" );
											}
										}
										else
											logStr( 0, "ERROR: Blend map size mismatch %dx%d.\n", w, h );
									}
									else
										logStr( 0, "ERROR: Wrong bitmap format. 24 or 32 bit needed!\n" );
								}
								else
									logStr( 0, "ERROR: BMP load read error.\n" );
							}
							else
								logStr( 0, "ERROR: No previous warp map found!.\n" );
						}
						else
							logStr( 0, "ERROR: Unknown file type \"%2c\" at %s(%u). Operation canceled.\n", h0.magicNumber, pp, ftell( f ) - sizeof( VWB_WarpSetFileHeader ) );
						ret = VWB_ERROR_VWF_LOAD;
						nSets = 0;
						break;
					}
				}
				else
					ret = VWB_ERROR_VWF_LOAD;
			}
			
			fclose(f);
			logStr( 1, "LoadVWF: Successfully added %u datasets to the mappings set.\n", VWB_uint(set.size()-nBefore) );
		}
		else
		{
			logStr( 0, "ERROR: LoadVWF: Error %d at open \"%s\"\n", err, pp );
			ret = VWB_ERROR_VWF_FILE_NOT_FOUND;
			break;
		}
		if( pP2 )
			pP = pP2 + 1;

	}while( pP2 );

	return ret;
}

VWB_ERROR WriteBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, FILE* f )
{
	if( nullptr == map || nullptr == f )
		return VWB_ERROR_PARAMETER;

	if( h.flags & FLAG_SP_WARPFILE_HEADER_BLENDV3 )
	{
		// TODO: add 
		return VWB_ERROR_NOT_IMPLEMENTED;
	}
	else if( h.flags & FLAG_SP_WARPFILE_HEADER_BLENDV2 )
	{
		// TODO: add 
		return VWB_ERROR_NOT_IMPLEMENTED;
	}
	else
	{
		const VWB_uint pitchBM = ( ( h.width * 24 + 31 ) / 32 ) * 4;
		BITMAPINFOHEADER bmih = { 
			sizeof( BITMAPINFOHEADER ), 
			h.width, 
			-h.height, 
			1, 24, 0, 
			h.height * pitchBM, 
			5512, 5512, 0, 0 };
		long a = ftell( f );
		BITMAPFILEHEADER bmfh = { 'MB', sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage, 0, 0, sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) };
		fwrite( &bmfh, sizeof( bmfh ), 1, f );
		fwrite( &bmih, sizeof( bmih ), 1, f );
		VWB_uint s = 0;
		for( VWB_BlendRecord const* p = map, *pE = map + h.width * h.height; p != pE; p++ )
		{
			VWB_byte c[3] = { p->b, p->g, p->r };
			s+= (VWB_uint)fwrite( c, 3, 1, f );
		}
		if( s * 3 != bmih.biSizeImage )
			throw - 1;
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, char const* path )
{
	char pp[MAX_PATH];
	strcpy_s( pp, path );
	MkPath( pp, MAX_PATH, ".vwf" );
	VWB_WarpSetFileHeader hdrSet = { {'v','w','f','1'}, 0, sizeof( VWB_WarpSetFileHeader ), 0 };
	// count blocks
	for( auto setIt : set )
	{
		if( setIt->pWarp )
			hdrSet.numBlocks++;
		else
			return VWB_ERROR_GENERIC;
		if( setIt->pBlend )
			hdrSet.numBlocks++;
		if( setIt->pBlack )
			hdrSet.numBlocks++;
		if( setIt->pWhite )
			hdrSet.numBlocks++;
	}
	if( hdrSet.numBlocks )
	{
		FILE* f = nullptr;
		if( NO_ERROR == fopen_s( &f, pp, "wb" ) && nullptr != f )
		{
			fwrite( &hdrSet, sizeof( hdrSet ), 1, f );
			for( auto setIt : set )
			{
				if( setIt->pWarp )
				{
					setIt->header.szHdr = sizeof( setIt->header );
					fwrite( &setIt->header, sizeof( setIt->header ), 1, f );
					fwrite( setIt->pWarp, sizeof( VWB_WarpRecord ), setIt->header.width * setIt->header.height, f );
				}
				long a = ftell( f );
				if( setIt->pBlend )
				{
					WriteBMP( setIt->header, setIt->pBlend, f );
				}
				if( setIt->pBlack )
				{
					WriteBMP( setIt->header, setIt->pBlack, f );
				}
				if( setIt->pWhite )
				{
					WriteBMP( setIt->header, setIt->pWhite, f );
				}
			}
			fclose( f );
			return VWB_ERROR_NONE;
		}
		else
		{
			logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		}
	}
	else
	{
		logStr( 0, "ERROR: SaveVWF: no maps in set." );
	}
	return VWB_ERROR_GENERIC;
}

VWB_rect& operator+=(VWB_rect& me, VWB_rect const& other )
{
	if( 0 == me.left && 0 == me.top && 0 == me.right && 0 == me.bottom )
		me = other;
	else
	{
		if( me.left > other.left )
			me.left = other.left;
		if( me.right < other.right )
			me.right = other.right;
		if( me.top > other.top )
			me.top = other.top;
		if( me.bottom < other.bottom )
			me.bottom = other.bottom;
	}
	return me;
}

VWB_rect operator+(VWB_rect const& me, VWB_rect const& other )
{
	VWB_rect r;
	if( 0 == me.left && 0 == me.top && 0 == me.right && 0 == me.bottom )
		r = other;
	else
	{
		if( me.left > other.left )
			r.left = other.left;
		else
			r.left = me.left;

		if( me.right < other.right )
			r.right = other.right;
		else
			r.right = me.right;

		if( me.top > other.top )
			r.top = other.top;
		else
			r.top = me.top;

		if( me.bottom < other.bottom )
			r.bottom = other.bottom;
		else
			r.bottom = me.bottom;
	}
	return r;
}

bool VerifySet( VWB_WarpBlendSet& set )
{
	for( VWB_WarpBlendSet::iterator it = set.begin(); it != set.end(); )
	{
		// check
		if( 0 != (*it)->header.hMonitor &&
			NULL != (*it)->pWarp &&
			0 != (*it)->header.width &&
			0 != (*it)->header.height )
		{
			if( (*it)->header.flags & FLAG_SP_WARPFILE_HEADER_DISPLAY_SPLIT )
			{
				if( (*it)->header.flags & FLAG_SP_WARPFILE_HEADER_OFFSET )
				{
					// we have a valid split, now look if we need to scale according to own compound
					// this happens, if we create several compounds on a mosaic, i.e. a power wall where there is just one big desktop monitor
					// find compound rect
					VWB_rect rCompound = {0};
					VWB_rect rScreen = rCompound;
					for( VWB_WarpBlendSet::iterator it2 = set.begin(); it2 != set.end(); it2++ )
					{
						if( ( (*it2)->header.flags & FLAG_SP_WARPFILE_HEADER_OFFSET ) &&
							0 != (*it2)->header.hMonitor )
						{
							VWB_rect rDisplay = { (VWB_int)(*it2)->header.offsetX, (VWB_int)(*it2)->header.offsetY, (VWB_int)(*it2)->header.offsetX + (VWB_int)(*it2)->header.width, (VWB_int)(*it2)->header.offsetY + (VWB_int)(*it2)->header.height };
							if( (*it)->header.hMonitor == (*it2)->header.hMonitor ) 
								rScreen+= rDisplay;
							if( 0 == strcmp( (*it)->path, (*it2)->path ) )
								rCompound+= rDisplay;
						}
					}
					if( ( rCompound.right - rCompound.left ) < ( rScreen.right - rScreen.left ) ||
						( rCompound.bottom - rCompound.top ) < ( rScreen.bottom - rScreen.top ) )
					{
						// this is where we have to do something
						float scaleX = float(rCompound.right-rCompound.left) / float(rScreen.right-rScreen.left);
						float scaleY = float(rCompound.bottom-rCompound.top) / float(rScreen.bottom-rScreen.top);
						float offsX  = float(rCompound.left-rScreen.left) / float(rScreen.right-rScreen.left);
						float offsY  = float(rCompound.top-rScreen.top) / float(rScreen.bottom-rScreen.top);
						for( VWB_WarpRecord* pDst = (*it)->pWarp, *pDstE = pDst + (*it)->header.width * (*it)->header.height; pDst != pDstE; pDst++ )
						{
							if( 1 == pDst->z )
							{
								pDst->x*= scaleX;
								pDst->y*= scaleY;
								pDst->x+= offsX;
								pDst->y+= offsY;
							}
						}
					}
				}
			}
			else
			{
				// try to merge if display is already there
				for( VWB_WarpBlendSet::iterator it2 = it + 1; it2 != set.end(); )
				{
					if( (*it)->header.hMonitor == (*it2)->header.hMonitor &&
						0 != (*it2)->header.width &&
						0 != (*it2)->header.height &&
						(*it)->header.width == (*it2)->header.width &&
						(*it)->header.height == (*it2)->header.height ) 
					{
						int iBlend = (*it)->pBlend && (*it2)->pBlend ? 1 : 0;
						VWB_WarpRecord* pW1 = (*it)->pWarp;
						VWB_WarpRecord* pW2 = (*it2)->pWarp;
						VWB_BlendRecord* pB1 = (*it)->pBlend;
						VWB_BlendRecord* pB2 = (*it2)->pBlend;

						// try other mappings
						for( VWB_WarpRecord const* pW1E = pW1 + (*it)->header.width * (*it)->header.height; pW1 != pW1E; pW1++, pW2++, pB1+= iBlend, pB2+=iBlend )
						{
							if( pW2->w > 0.0f ) // other set is filled here...
							{
								// if this set is not filled here
								if( pW1->w == 0.0f )
								{
									*pW1 = *pW2;
									if( pB1 && pB2 )
										*pB1 = *pB2;
								} 
								//TODO find uniformly filled parts
								// if this set is filled unifomly and the other is not
								//else if( 
							}
						}
						it2 = set.erase( it2 ); // does not invalitate it!
					}
					else
						it2++;
				}
			}
			it++;
		}
		else
			it = set.erase( it );
	}
	return !set.empty();
}

VWB_ERROR ScanVWF( char const* path, VWB_WarpBlendHeaderSet* set )
{
	if( NULL == set )
	{
		logStr( 0, "ERROR: VWB_vwfInfo: set is NULL.\n" );
		return VWB_ERROR_PARAMETER;
	}
	DeleteVWF( *set );

	if( NULL == path )
		return VWB_ERROR_NONE;

	if( 0 == *path )
	{
		logStr( 0, "ERROR: VWB_vwfInfo: Path to warp file empty.\n" );
		return VWB_ERROR_PARAMETER;
	}

	VWB_ERROR ret = VWB_ERROR_NONE;
	char const* pP = path;
	char pp[MAX_PATH];
	char const* pP2;
	do
	{
		pP2 = strchr( pP, ',' );
		if( pP2 )
			strncpy_s( pp, pP, pP2 - pP );
		else
			strcpy( pp, pP );
		MkPath( pp, MAX_PATH, ".vwf" );

		logStr( 2, "Open \"%s\"...\n", pp );
		FILE* f = NULL;
		errno_t err = NO_ERROR;
		if( NO_ERROR == ( err = fopen_s( &f, pp, "rb" ) ) )
		{
			logStr( 2, "File found and openend.\n" );
			VWB_WarpSetFileHeader h0;
			int nSets = 1;
			while( nSets )
			{
				if( 1 == fread_s( &h0, sizeof( VWB_WarpSetFileHeader ), sizeof( VWB_WarpSetFileHeader ), 1, f ) )
				{
					switch( *(VWB_uint*)&h0.magicNumber )
					{
					case '3fwv':
					case '2fwv':
					case '0fwv':
						{
							logStr( 2, "Found map header %d...\n", VWB_uint(set->size()) );
							VWB_WarpBlendHeader* pWBH = new VWB_WarpBlendHeader;
							memset( pWBH, 0, sizeof( VWB_WarpBlendHeader ) );
							memcpy( pWBH, &h0, sizeof( h0 ) );
							// read remainder of header
							size_t sh = sizeof( pWBH->header );
							if( sh > pWBH->header.szHdr )
								sh = pWBH->header.szHdr;
							size_t s1 = sizeof( h0 );
							size_t s2 = sizeof( pWBH->header );
							if( 1 == fread_s( ((char*)&pWBH->header) + s1, s2 - s1, sh - s1, 1, f ) )
							{
								fseek( f, (long)pWBH->header.szHdr - (long)sh, SEEK_CUR );
								int nRecords = pWBH->header.width * pWBH->header.height;
								if( 0 != nRecords && 0 != pWBH->header.hMonitor )
								{
									strcpy( pWBH->path, pp );
									if( 0 == fseek( f, nRecords * sizeof( VWB_WarpRecord ), SEEK_CUR ) )
									{
										set->push_back( pWBH );
										logStr( 2, "Map header read successfully (%dx%d).\n", pWBH->header.width, pWBH->header.height );
										nSets--;
										break;
									}
									else
										logStr( 0, "ERROR: Malformed file data.\n" );
								}
								else
									logStr( 0, "ERROR: Malformed file header.\n" );
							}
							else
								logStr( 0, "ERROR: Unexpected end of file.\n" );
							ret = VWB_ERROR_VWF_LOAD;
							nSets = 0;
							delete pWBH;
							break;
						}
						break;
					case '1fwv':
						// load a set
						nSets = h0.numBlocks;
						fseek( f, h0.offs, SEEK_SET );
						logStr( 2, "File contains %d chunks.\n", nSets );
						break;
					default:
						if( 'B' == h0.magicNumber[0] && 'M' == h0.magicNumber[1] )
						{ // load bitmap
							if( 0 != set->size() )
							{
								logStr( 2, "Skip bitmap for dataset %d...\n", VWB_uint(set->size()) );
								BITMAPFILEHEADER& bmfh = *(BITMAPFILEHEADER*)&h0; // as BITMAPFILEHEADER is smaller than vwf header
								// bmfh.bfOffBits: offset of actual data
								// fill BITMAPINFOHEADER:
								BITMAPINFOHEADER bmih;
								// fill first bytes with last bytes of vwf file header
								VWB_uint s1 = sizeof( h0 );
								VWB_uint s2 = sizeof(bmfh);
								VWB_uint s3 = sizeof(bmih);
								memcpy( (VWB_byte*)&bmih, ((VWB_byte*)&h0)+s2, s1 - s2 );
								// read remaining from file
								if( 1 == fread_s( 
									((VWB_byte*)&bmih) + s1 - s2,
									s3 - s1 + s2,
									s3 - s1 + s2, 1, f ) )
								{
									if( bmfh.bfOffBits != s2 + s3 )
										fseek( f, (long)bmfh.bfOffBits - (long)s2 - (long)s3, SEEK_CUR );
									if( 24 == bmih.biBitCount ||
										32 == bmih.biBitCount )
									{
										int w = bmih.biWidth;
										int h = abs( bmih.biHeight );
										if( h == (VWB_int)set->back()->header.height &&
											w == (VWB_int)set->back()->header.width )
										{
											int bmPitch = (( w * bmih.biBitCount + 31 ) / 32 ) * 4;
											int bmSize = bmPitch * h;
											if( 0 == fseek( f, bmSize, SEEK_CUR ) )
											{
												nSets--;
												if( bmSize + bmfh.bfOffBits != bmfh.bfSize )
												{
													fseek( f, bmfh.bfSize - bmfh.bfOffBits - bmSize, SEEK_CUR );
												}
												break;
											}
											else
												logStr( 0, "ERROR: Unexpected end of file. Bitmap or vwf file broken!\n" );
										}
										else
											logStr( 0, "ERROR: Blend map size mismatch %dx%d.\n", w, h );
									}
									else
										logStr( 0, "ERROR: Wrong bitmap format. 24 or 32 bit needed!\n" );
								}
								else
									logStr( 0, "ERROR: BMP load read error.\n" );
							}
							else
								logStr( 0, "ERROR: No previous warp map found!.\n" );
						}
						else
							logStr( 0, "ERROR: Unknown file type \"%2c\" at %s(%u). Operation canceled.\n", h0.magicNumber, pp, ftell( f ) - sizeof( VWB_WarpSetFileHeader ) );
						//ret = VWB_ERROR_VWF_LOAD; TO
						nSets = 0;
						break;
					}
				}
				else
					ret = VWB_ERROR_VWF_LOAD;
			}
			
			fclose(f);
			logStr( 1, "VWB_vwfInfo: Successfully analyzed %u datasets.\n", VWB_uint(set->size()) );

			// delete same hMon
			for( VWB_WarpBlendHeaderSet::iterator it = set->begin(); it != set->end(); )
			{
				// check
				if( 0 != (*it)->header.hMonitor &&
					0 != (*it)->header.width &&
					0 != (*it)->header.height )
				{
					if( !((*it)->header.flags & FLAG_SP_WARPFILE_HEADER_DISPLAY_SPLIT) )
					{
						for( VWB_WarpBlendHeaderSet::iterator it2 = it + 1; it2 != set->end(); )
						{
							if( (*it)->header.hMonitor == (*it2)->header.hMonitor &&
								0 != (*it2)->header.width &&
								0 != (*it2)->header.height &&
								(*it)->header.width == (*it2)->header.width &&
								(*it)->header.height == (*it2)->header.height ) 

								it2 = set->erase( it2 ); // does not invalitate it!
							else
								it2++;
						}
					}
					it++;
				}
				else
					it = set->erase( it );
			}

		}
		else
		{
			logStr( 0, "ERROR: LoadVWF: Error %d at open \"%s\"\n", err, pp );
			ret = VWB_ERROR_VWF_FILE_NOT_FOUND;
			break;
		}
		if( pP2 )
			pP = pP2 + 1;

	}while( pP2 );

	return ret;
}

VWB_ERROR CreateDummyVWF( VWB_WarpBlendSet& set, char const* path )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: LoadVWF: Missing path to warp file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	VWB_ERROR ret = VWB_ERROR_NONE;
	char const* pP = path;
	char pp[MAX_PATH];
	char const* pP2;
	do
	{
		VWB_WarpBlendSet::size_type nBefore = 0;
		pP2 = strchr( pP, ',' );
		if( pP2 )
			strncpy_s( pp, pP, pP2 - pP );
		else
			strcpy( pp, pP );
		MkPath( pp, MAX_PATH, ".vwf" );

		// create basic set, no warping no blending
		set.push_back( new VWB_WarpBlend );
		static const VWB_WarpFileHeader4 wfh = {
			{'v','w','f','0'},
			sizeof( VWB_WarpFileHeader4 ),
			FLAG_SP_WARPFILE_HEADER_BLENDV2|FLAG_SP_WARPFILE_HEADER_CALIBRATION_BASE_TYP|FLAG_SP_WARPFILE_HEADER_OFFSET|FLAG_SP_WARPFILE_HEADER_BLACKLEVEL_CORR,
			0x10001,
			256*256*sizeof( VWB_WarpRecord ),
			256,	256,
			{1,1,1,1},
			{0,0,0,0},
			0,0,0,0,0,0,
			TYP_SP_CALIBBASE_DISPLAY_COMPOUND,
			0,0,
			0,0,0,
			0,
			{0,0,0},
			"dummy",
			{0},
			0,
			{0,0,255,255,1.0f/256,1.0f/256,256*256},
			{0,0,1,1,1,0,0,1,1},
			{0},
			{0},
			{0},
			"localhost"
		};
		set.back()->header = wfh;
		strcpy( set.back()->path, pp );
		set.back()->pBlend2 = new VWB_BlendRecord2[256*256];
		for( VWB_BlendRecord2* pB = set.back()->pBlend2, *pBE = pB + 256 * 256; pB != pBE; pB++ )
			pB->r = pB->g = pB->b = pB->a = 65535;

		VWB_WarpRecord* pW = set.back()->pWarp = new VWB_WarpRecord[256*256];
		for( int y = 0; y != 256; y++ )
			for( int x = 0; x != 256; x++, pW++ )
			{
				pW->x = float(x)/255;
				pW->y = float(y)/255;
				pW->z = 1;
				pW->w = 0;
			}
		set.back()->pBlack = NULL;
		set.back()->pWhite = NULL;
		logStr( 2, "INFO: CreateDummyVWF: Set created.\n" );
		if( pP2 )
			pP = pP2 + 1;

	}while( pP2 );
	return ret;
}
