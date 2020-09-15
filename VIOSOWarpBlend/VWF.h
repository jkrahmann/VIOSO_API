#include "../Include/VWBTypes.h"
#include <iostream>

/// @brief free memory used by a VWB_WarpBlend structure
/// @param free memory used by a VWB_WarpBlend structure
bool DeleteVWF( VWB_WarpBlend& wb );
bool DeleteVWF( VWB_WarpBlendSet& set );
bool DeleteVWF( VWB_WarpBlendHeaderSet& set );
VWB_ERROR LoadVWF( VWB_WarpBlendSet& set, char const* path );
VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, std::ostream& os );
VWB_ERROR SaveBMP_RGBA( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, char const* path );
///< @remark use SaveBMP   VWB_WarpRecord only for debug reasons, it will save 8 bit RGB only!
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_WarpRecord const* map, std::ostream& os ); 
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_WarpRecord const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, std::ostream& os );
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord2 const* map, char const* path );
VWB_ERROR SaveBMP( VWB_WarpFileHeader4 const& h, VWB_BlendRecord2 const* map, std::ostream& os );
VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, std::ostream& os );
VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, char const* path );
bool VerifySet( VWB_WarpBlendSet& set );
VWB_ERROR ScanVWF( char const* path, VWB_WarpBlendHeaderSet* set );

/// @brief upgrade set to be loaded to shader
/// * blend gets changed into U16 format
/// * validity channel (warp.z for 2D or w for 3D mappings) is copied to blend.a.
/// * gamma is applied to blend
/// * an empty blend map is created, in case there is no blend map
/// @param [IN|OUT] set				the set to upgrade
/// @param [IN|OPT] gamma  the used gamma value
/// @return VWB_ERROR_NONE in case of success 
VWB_ERROR PrepareForUse( VWB_WarpBlend& wb, const float gamma = 0 );

/// @brief creates an unwarped mapping and adds it to the given set. All warp maps are "bypass", that means they mimic an unwarped display, all pixels used, no blend
/// @param [OUT] set				the set to add
/// @param [IN] path				the path, the mapping was loaded from, must not be empty. Will be expanded to a valid path. Can be used later to save.
/// @param [IN] xPos, yPos			the desktop position (upper left corner) of the display part
/// @param [IN] width, height       size of the display in desktop coordinates
/// @param [IN] displayName			the name of the display
/// @param [IN] splitW				the number of columns in the split
/// @param [IN] splitH				the number of rows in the split
/// @param [IN] splitX				the split display's column index
/// @param [IN] splitY				the split display's row index
/// @return VWB_ERROR_NONE in cas of success, VWB_ERROR_PARAMETER if some parameter is out of bound
VWB_ERROR AddUnwarped2DTo( VWB_WarpBlendSet& set, const char* path, int xPos, int yPos, int width, int height, const char* displayName, int splitW, int splitH, int splitX, int splitY );
