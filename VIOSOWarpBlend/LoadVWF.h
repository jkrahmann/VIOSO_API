#include "../Include/VWBTypes.h"

bool DeleteVWF( VWB_WarpBlend& wb );
bool DeleteVWF( VWB_WarpBlendSet& set );
bool DeleteVWF( VWB_WarpBlendHeaderSet& set );
VWB_ERROR LoadVWF( VWB_WarpBlendSet& set, char const* path );
VWB_ERROR SaveVWF( VWB_WarpBlendSet const& set, char const* path );
bool VerifySet( VWB_WarpBlendSet& set );
VWB_ERROR ScanVWF( char const* path, VWB_WarpBlendHeaderSet* set );
VWB_ERROR CreateDummyVWF( VWB_WarpBlendSet& set, char const* path );
