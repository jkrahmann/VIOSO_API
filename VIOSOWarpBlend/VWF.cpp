#include "Platform.h"

#include "VWF.h"
#include "logging.h"
#include "PathHelper.h"

#include <stdlib.h>
#include <fstream>

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
			// strncpy_s( pp, pP, pP2 - pP );
            strncpy( pp, pP, pP2 - pP );
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
								size_t s1 = sizeof( h0 );
								size_t s2 = sizeof(bmfh);
								size_t s3 = sizeof(bmih);
								memcpy( (VWB_byte*)&bmih, ((VWB_byte*)&h0)+s2, s1 - s2 );
								// read remaining from file
								if( 1 == fread_s( 
									((VWB_byte*)&bmih) + s1 - s2,
									s3 - s1 + s2,
									s3 - s1 + s2, 1, f ) )
								{
									if( bmfh.bfOffBits != s2 + s3 )
										fseek( f, (long)bmfh.bfOffBits - (long)s2 - (long)s3, SEEK_CUR );
									if( 64 == bmih.biBitCount ||
										48 == bmih.biBitCount ||
										24 == bmih.biBitCount ||
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
											VWB_BlendRecord* pB = nullptr; 
											VWB_BlendRecord2* pB2 = nullptr;
											if( 32 < bmih.biBitCount )
											{
												pB2 = new VWB_BlendRecord2[n];
											}
											else
											{
												pB = new VWB_BlendRecord[n];
											}
											char* pBMData = new char[bmSize];
											if( h == (int)fread_s( pBMData, bmSize, bmPitch, h, f ) )
											{
												if( bTopDown )
												{
													if( 64 == bmih.biBitCount )
														memcpy( pB2, pBMData, n * sizeof( VWB_BlendRecord2 ) ); // trivial, just copy the whole buffer
													else if( 32 == bmih.biBitCount )
														memcpy( pB, pBMData, n * sizeof( VWB_BlendRecord ) ); // trivial, just copy the whole buffer
													else if( 48 == bmih.biBitCount )
													{
														char* pBML = pBMData;
														for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pBML += bmPadding )
														{
															for( VWB_BlendRecord2* pLLE = pL + w; pL != pLLE; pL++, pBML += 6 )
															{
																VWB_word* pW = (VWB_word*)pBML;
																pL->r = pW[2];
																pL->g = pW[1];
																pL->b = pW[0];
																pL->a = 65535;
															}
														}
													}
													else if( 24 == bmih.biBitCount )
													{
														char* pBML = pBMData;
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pBML += bmPadding )
															for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML += 3 )
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
													if( 64 == bmih.biBitCount )
													{ // reverse order
														char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
														for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch ) // padding is always 0
															memcpy( pL, pBML, n * sizeof( VWB_BlendRecord ) );
													}
													else if( 48 == bmih.biBitCount )
													{
														char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
														for( VWB_BlendRecord2* pL = pB2, *pLE = pB2 + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch - (ptrdiff_t)bmPadding )
														{
															for( VWB_BlendRecord2* pLLE = pL + w; pL != pLLE; pL++, pBML += 6 )
															{ // todo: interpret bitfield mask here
																VWB_word* pW = (VWB_word*)pBML;
																pL->r = pW[2];
																pL->g = pW[1];
																pL->b = pW[0];
																pL->a = 65535;
															}
														}
													}
													else if( 32 == bmih.biBitCount )
													{ // reverse order
														char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch ) // padding is always 0
															memcpy( pL, pBML, n * sizeof( VWB_BlendRecord ) );
													}
													else 
													{
														char* pBML = pBMData + (ptrdiff_t)bmPitch * ( (ptrdiff_t)h - 1 );
														for( VWB_BlendRecord* pL = pB, *pLE = pB + n; pL != pLE; pL += w, pBML -= (ptrdiff_t)bmPitch - (ptrdiff_t)bmPadding )
														{
															for( VWB_BlendRecord* pLLE = pL + w; pL != pLLE; pL++, pBML += 3 )
															{ // todo: interpret bitfield mask here
																pL->r = pBML[2];
																pL->g = pBML[1];
																pL->b = pBML[0];
																pL->a = 255;
															}
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
			logStr( 0, "ERROR: LoadVWF: Error %d at open \"%s\". Missing file?\n", err, pp );
			ret = VWB_ERROR_VWF_FILE_NOT_FOUND;
			break;
		}
		if( pP2 )
			pP = pP2 + 1;

	}while( pP2 );

	return ret;
}


VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = h.width * 4;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 32, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0 
	};
	BITMAPFILEHEADER bmfh = {
		'MB', 
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0, 
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) 
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; p++ )
	{
		VWB_byte c[4] = { p->b, p->g, p->r, 255 };
		os.write( (const char*)c, 4 );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_WarpRecord const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	// bmp stores pixels in packed rows, the size of each row is rounded up to a multiple of 4 bytes by padding
	// pitchBM = number of bytes necessary to store one row of pixels = rounded up to a multiple of 4
	const VWB_uint pitchBM = ( ( h.width * 24 + 31 ) / 32 ) * 4;
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),		// number of bytes required by the structure
		h.width,
		-h.height,						// biHeight is negative, the bitmap is a top-down DIB with the origin at the upper left corner.
		1,								// biPlanes, Specifies the number of planes for the target device. This value must be set to 1.
		24,								// biBitCount, Specifies the number of bits per pixel (bpp).
		0,								// biCompression
		0,								// biSizeImage, Specifies the size, in bytes, of the image. This can be set to 0 for uncompressed RGB bitmaps.
		5512,							// biXPelsPerMeter, horizontal resolution, in pixels per meter, 
		5512,							// biYPelsPerMeter, horizontal resolution, in pixels per meter, 
		0,								// biClrUsed, number of color indices in the color table 
		0								// biClrImportant, number of color indices that are considered important for displaying the bitmap. If this value is zero, all colors are important.
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	// iterate through the whole image line by line. 
	for( VWB_WarpRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE;)
	{
		// write one line, write until we are at the end
		for( VWB_WarpRecord const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_byte c[3] = { (VWB_byte)( 255.0f * p->z ), (VWB_byte)( 255.0f * p->y ), (VWB_byte)( 255.0f * p->x ) };
			os.write( (const char*)c, 3 );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_WarpRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	//strcpy_s( pp, path );
	strcpy(pp, path);

	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.bad() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = ( ( h.width * 24 + 31 ) / 32 ) * 4;
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 24, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; )
	{
		for( VWB_BlendRecord const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_byte c[3] = { p->b, p->g, p->r };
			os.write( (const char*)c, 3 );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord2 const* map, std::ostream& os )
{
	if( nullptr == map || os.bad() )
		return VWB_ERROR_PARAMETER;

	const VWB_uint pitchBM = ( ( h.width * 48 + 31 ) / 32 ) * 4;
	const VWB_uint paddingBM = pitchBM - h.width * 3;
	BITMAPINFOHEADER bmih = {
		sizeof( BITMAPINFOHEADER ),
		h.width,
		-h.height,
		1, 48, 0,
		h.height * pitchBM,
		5512, 5512, 0, 0
	};
	BITMAPFILEHEADER bmfh = {
		'MB',
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih ) + bmih.biSizeImage,
		0, 0,
		sizeof( BITMAPFILEHEADER ) + sizeof( bmih )
	};
	os.write( (const char*)&bmfh, sizeof( bmfh ) );
	os.write( (const char*)&bmih, sizeof( bmih ) );
	for( VWB_BlendRecord2 const* p = map, *pE = map + (ptrdiff_t)h.width * (ptrdiff_t)h.height; p != pE; )
	{
		for( VWB_BlendRecord2 const* pLE = p + h.width; p != pLE; p++ )
		{
			VWB_word c[3] = { p->b, p->g, p->r };
			os.write( (const char*)c, 3 * sizeof( p->r ) );
		}
		os.write( "\0\0\0", paddingBM );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	// strcpy_s( pp, path );
    strcpy( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.bad() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP_RGBA( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, char const* path )
{
	char pp[MAX_PATH];
	//strcpy_s( pp, path );
	strcpy( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.bad() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord2 const* map, char const* path )
{
	char pp[MAX_PATH];
	//strcpy_s( pp, path );
	strcpy( pp, path );
	MkPath( pp, MAX_PATH, ".bmp" );
	std::ofstream os( pp, std::ios_base::binary );
	if( os.bad() )
	{
		logStr( 0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp );
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveBMP( h, map, os );
}

VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, std::ostream& os )
{
	if (os.bad())
		return VWB_ERROR_PARAMETER;

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
		os.write( (const char*)&hdrSet, sizeof( hdrSet ) );
		for( auto setIt : set )
		{
			if( setIt->pWarp )
			{
				setIt->header.szHdr = sizeof( setIt->header );
				os.write( (const char*)&setIt->header, sizeof( setIt->header ) );
				os.write( (const char*)setIt->pWarp, sizeof( VWB_WarpRecord ) * size_t( setIt->header.width ) * setIt->header.height );
			}
			if( setIt->pBlend )
			{
				SaveBMP( setIt->header, setIt->pBlend, os );
			}
			if( setIt->pBlack )
			{
				SaveBMP( setIt->header, setIt->pBlack, os );
			}
			if( setIt->pWhite )
			{
				SaveBMP( setIt->header, setIt->pWhite, os );
			}
			if( os.bad() )
			{
				logStr( 0, "ERROR: SaveVWF: Error writing.\n" );
				return VWB_ERROR_VWF_FILE_NOT_FOUND;
			}

		}
		return VWB_ERROR_NONE;
	}
	else
	{
		logStr( 0, "ERROR: SaveVWF: no maps in set, nothing to save." );
		return VWB_ERROR_PARAMETER;
	}
	return VWB_ERROR_GENERIC;
}

VWB_ERROR SaveVWF(VWB_WarpBlendSet const& set, char const* path)
{
	char pp[MAX_PATH];
	//strcpy_s(pp, path);
	strcpy(pp, path);
	MkPath(pp, MAX_PATH, ".vwf");
	std::ofstream os( pp, std::ios_base::binary );
	if (os.bad())
	{
		logStr(0, "ERROR: SaveVWF: Error opening \"%s\"\n", pp);
		return VWB_ERROR_VWF_FILE_NOT_FOUND;
	}

	return SaveVWF(set, os);
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
			if( (*it)->header.flags & FLAG_WARPFILE_HEADER_DISPLAY_SPLIT )
			{
				if( (*it)->header.flags & FLAG_WARPFILE_HEADER_OFFSET )
				{
					// we have a valid split, now look if we need to scale according to own compound
					// this happens, if we create several compounds on a mosaic, i.e. a power wall where there is just one big desktop monitor
					// find compound rect
					VWB_rect rCompound = {0};
					VWB_rect rScreen = rCompound;
					for( VWB_WarpBlendSet::iterator it2 = set.begin(); it2 != set.end(); it2++ )
					{
						if( ( (*it2)->header.flags & FLAG_WARPFILE_HEADER_OFFSET ) &&
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
						for( VWB_WarpRecord* pDst = (*it)->pWarp, *pDstE = pDst + (ptrdiff_t)(*it)->header.width * (ptrdiff_t)(*it)->header.height; pDst != pDstE; pDst++ )
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
						for( VWB_WarpRecord const* pW1E = pW1 + (ptrdiff_t)(*it)->header.width * (ptrdiff_t)(*it)->header.height; pW1 != pW1E; pW1++, pW2++, pB1+= iBlend, pB2+=iBlend )
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
			strncpy( pp, pP, pP2 - pP );
			//strncpy_s( pp, pP, pP2 - pP );
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
								size_t s1 = sizeof( h0 );
								size_t s2 = sizeof(bmfh);
								size_t s3 = sizeof(bmih);
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
					if( !((*it)->header.flags & FLAG_WARPFILE_HEADER_DISPLAY_SPLIT) )
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

VWB_ERROR AddUnwarped2DTo( VWB_WarpBlendSet& set, const char* path, int xPos, int yPos, int width, int height, const char* displayName, int splitW, int splitH, int splitX, int splitY )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Missing path to warp file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= width || 0 >= height )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Need positive dimension.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( NULL == displayName || 0 == *displayName )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: Missing display name.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 >= splitW || 0 >= splitH )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: split grid size ot of range.\n" );
		return VWB_ERROR_PARAMETER;
	}
	if( 0 > splitX || splitW <= splitX || 0 > splitY || splitH <= splitY )
	{
		logStr( 0, "ERROR: AddUnwarped2DToVWF: split grid index ot of range.\n" );
		return VWB_ERROR_PARAMETER;
	}

	VWB_ERROR ret = VWB_ERROR_NONE;
	char pp[MAX_PATH];
	//strcpy_s( pp, path );
	strcpy( pp, path );
	MkPath( pp, MAX_PATH, ".vwf" );

	// create basic set, no warping no blending
	VWB_uint nRecords = (VWB_uint)width * (VWB_uint)height;
	set.push_back( new VWB_WarpBlend() );
	set.back()->header = {
            {'v','w','f','0'},
            (VWB_uint)0,
            (VWB_uint)0,
            (VWB_uint)0,
            (VWB_uint)0,
            (VWB_int)0,
            (VWB_int)0,
            VWB_float{0},
            VWB_float{0},
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float(0.f),
            VWB_float{0},
            char{0},
            char{0},
            VWB_ull(0),
            VWB_float{0},
            VWB_float{0},
            char{0},
            VWB_float{0},
            VWB_wchar{0},
            char{0}
	};

	        /*{
		{'v','w','f','0'},
		(VWB_uint)sizeof( VWB_WarpFileHeader4 ),
		FLAG_WARPFILE_HEADER_CALIBRATION_BASE_TYP|FLAG_WARPFILE_HEADER_OFFSET|FLAG_WARPFILE_HEADER_BLACKLEVEL_CORR,
		0x10001,
        (VWB_uint)(nRecords * sizeof( VWB_WarpRecord )),
		width,	height,
		{1.f,1.f,1.f,1.f},
		{0.f,0.f,0.f,0.f},
		(VWB_float)splitY,(VWB_float)splitX,(VWB_float)splitH,(VWB_float)splitW,VWB_float( splitW * width ),VWB_float( splitH * height ),
		TYP_CALIBBASE_DISPLAY_COMPOUND,
		(VWB_float)xPos,(VWB_float)yPos,
		0.f,0.f,0.f,
		0.f,
		{0.f,0.f,0.f},
		"dummy",
		{0},
		0,
		{0.f,0.f,(VWB_float)width,(VWB_float)height,1.0f/width,1.0f/height,VWB_float(width*height)},
		{0.f,0.f,1.f,1.f,1.f,0.f,0.f,1.f,1.f},
		{0},
		{0},
		{0},
		"localhost"
	};
	*/
	// set.back()->header = wfh;
	strcpy( set.back()->path, pp );

	set.back()->pBlend = new VWB_BlendRecord[nRecords];
	for( VWB_BlendRecord* pB = set.back()->pBlend, *pBE = pB + nRecords; pB != pBE; pB++ )
		pB->r = pB->g = pB->b = pB->a = 255;

	VWB_WarpRecord* pW = set.back()->pWarp = new VWB_WarpRecord[nRecords];
	int startX = splitX * width;
	int startY = splitY * height;
	int endX = startX + width;
	int endY = startY + height;

	int tW = splitW * width;
	int tH = splitH * height;

	VWB_float dX = 0.5f / tW; // midpix
	VWB_float dY = 0.5f / tH;
	VWB_float sX = 1.0f / VWB_float( tW );
	VWB_float sY = 1.0f / VWB_float( tH );
	for( int y = startY; y != endY; y++ )
	{
		VWB_float fy = VWB_float( y ) * sY + dY;
		for( int x = startX; x != endX; x++, pW++ )
		{
			pW->x = VWB_float( x ) * sX + dX;
			pW->y = fy;
			pW->z = 1;
			pW->w = 0;
		}
	}
	set.back()->pBlack = NULL;
	set.back()->pWhite = NULL;
	logStr( 2, "INFO: AddUnwarped2DToVWF: Set added.\n" );

	return ret;
}


