#ifndef VWB_PATH_HELPER_HPP
#define VWB_PATH_HELPER_HPP

#include "../Include/VWBTypes.h"

/// @brief creates a full canonical path from a filename
/// @description if it does not start with a / or a drive letter (no root path)
/// it is set relative to the module currently set in g_hModule
/// @param [IN|OUT] path the input path or file name
/// @param [IN] nMaxPath the maximum number of chars path can hold
/// @param [IN_OPT] ext an extension to add, set to 
/// @return the value of path, containing the formatted and canonical path,
/// NULL in case of an error
/// @remarks
/// removes leading and trailing whitespaces
/// removes leading single and double quotes and their counterparts
/// expands environment strings (Windows only)
/// removes double directory separators
/// removes dir/../
/// adds .ext in case \b ext isn't null and the extension isn't already present
char* 
MkPath(char* path, VWB_uint nMaxPath, char const* ext = NULL);

/// @brief reads a string from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] szDefault the default value
/// @param [OUT] s the returned string
/// @param [IN] sz the number of characters, the string can hold
/// @param [IN] szConfigFile the path to the ini file
/// @return true in case the key was found in given section
/// and a value was found
/// @remarks
/// if a section marker is present more than once in a file,
/// only the first section is browsed for keys
/// all \b szSection and \szKey must only contain upper and lower
/// case letters, names are case sensitive
/// the equal sign must follow directly in case of a 
/// key=value entry
/// in case a key is not found in the section, the default value is copied,
/// if szDefault is NULL, an empty string is returned
bool 
GetIniString(char const* szSection, char const* szKey, char* szDefault, char* s, VWB_uint sz, char const* szConfigFile);
template<size_t sz>
bool
GetIniString(char const* szSection, char const* szKey, char* szDefault, char(&s)[sz], char const* szConfigFile)
{ return GetIniString(szSection, szKey, szDefault, s, sz, szConfigFile); }

/// @brief reads an integer from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] iDefault the default value
/// @param [IN] szConfigFile the path to the ini file
/// @return the read value or the value given in iDefault
VWB_int
GetIniInt(char const* szSection, char const* szKey, VWB_int iDefault, char const* szConfigFile);

/// @brief reads a float from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] fDefault the default value
/// @param [IN] szConfigFile the path to the ini file
/// @return the read value or the value given in fDefault
VWB_float
GetIniFloat(char const* szSection, char const* szKey, VWB_float fDefault, char const* szConfigFile);

/// @brief reads a float vector or matrix from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] iDefault the default value
/// @param [IN] szConfigFile the path to the ini file
/// @return f in case of success; this includes , NULL otherwise
/// if fDefault is NULL, an empty matrix (all 0.0f) is returned
VWB_float*
GetIniMat(char const* szSection, char const* szKey, int dimX, int dimY, VWB_float const* fDefault, VWB_float* f, char const* szConfigFile);
template< int sz>
VWB_float*
GetIniMat(char const* szSection, char const* szKey, VWB_float const* fDefault, VWB_float(&f)[sz], char const* szConfigFile) 
{ return GetIniMat(szSection, szKey, sz, 1, fDefault, f, szConfigFile); }

#endif //ndef VWB_PATH_HELPER_HPP


