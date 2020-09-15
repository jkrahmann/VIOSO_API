#include "Platform.h"
#include "PathHelper.h"

#ifndef WIN32
#include <dlfcn.h>
#include <stdlib.h>
#endif

char* MkPath(char* path, VWB_uint nMaxPath, char const* ext)
{
	if (NULL == path || 0 == path[0])
		return NULL;

	// remove leading white spaces
	while (' ' == *path || '\t' == *path)
		path++;

	// remove trailing white spaces
	char* lnwsc = path + strlen(path);
	while (--lnwsc > path)
		if (' ' == *lnwsc || '\t' == *lnwsc)
			*lnwsc = 0;
		else
			break;

	// remove quotes
	lnwsc = path + strlen(path) - 1;
	if (lnwsc > path)
	{
		if ('"' == *path && '"' == *lnwsc)
		{
			path++;
			*lnwsc = 0;
		}
		else if ('\'' == *path && '\'' == *lnwsc)
		{
			path++;
			*lnwsc = 0;
		}
	}

	if (0 == path[0])
		return NULL;

#ifdef WIN32
	const char psc = '\\';
	{
		char* path2 = new char[nMaxPath];
		if (::ExpandEnvironmentStringsA(path, path2, nMaxPath))
			strcpy_s(path, nMaxPath, path2);
		delete[] path2;
	}
#else
	char psc = '/';
#endif //def WIN32

	size_t ss = strlen(path);
	if (NULL != ext &&
		0 != ext[0] &&
		(strlen(ext) >= ss || 0 != strcmp(ext, &path[ss - strlen(ext)])))
		strcat_s(path, nMaxPath, ext);
	// if not canonical path
#ifdef WIN32
	// has less than 2 char
	bool b1 = 3 > ss;
	// does start with a drive letter followed by :\ 
	bool b21 = 'a' <= path[0] && 'z' >= path[0];
	bool b22 = 'A' <= path[0] && 'Z' >= path[0];
	bool b23 = ':' == path[1] && '\\' == path[2];
	bool b2 = ((b21 || b22) && b23);
	// does start with \\ 
	bool b3 = '\\' == path[0] || '\\' == path[1];
	if (b1 || !(b2 || b3))
#else
	// ~X systems start with /
	if ('/' != path[0])
#endif //def WIN32
	{
		char szModPath[MAX_PATH] = { 0 };
#ifdef WIN32
		if (::GetModuleFileNameA(g_hModDll, szModPath, MAX_PATH))
		{
#else
		Dl_info dl_info;
		if (dladdr((void *)MkPath, &dl_info))
		{
			strcpy(szModPath, dl_info.dli_fname);
#endif //def WIN32

#ifdef WIN32
			// we start with a \ 
			if ('\\' == path[0])
			{ // add a my drive letter
				strcpy_s(&szModPath[3], MAX_PATH - 3, path);
				strcpy_s(path, nMaxPath, szModPath);
			}
			else
#endif
			{
				// add path from module
				char* pS = strrchr(szModPath, psc);
				if (pS++)
				{
					strcpy_s(pS, MAX_PATH - (pS - szModPath), path);
					strcpy_s(path, nMaxPath, szModPath);
				}
			}
		}
	}

	// remove all duplicate separators
	char *b, *a;
	for (a = path, b = path; *a; )
	{
		if (psc != *++a ||
			psc != *(a + 1))
		{
			*++b = *a;
		}
	}

	/* replace all \.\ with \ */
	for (a = path, b = path; *a; a++)
	{
		if (psc != *a ||
			'.' != *(a + 1) ||
			psc != *(a + 2))

			*b++ = *a;
		else
			a++;
	}
	*b = 0;

	/* replace all \path\..\ with \ */
	char* cs[MAX_PATH] = { 0 };
	char** c = cs;
	for (a = path; *a; a++)
		if (psc == *a)
		{
			*++c = a++;
			break;
		}
	for (b = a; *a; a++)
	{
		// keep a stack of \ positions
		if (psc == *a)
			*++c = a;
		// c contains position of second last \ if any
		 // if  is not set, this is a malformed path, we cannot replace...
		if (NULL == *(c - 1) ||
			'.' != *a ||
			'.' != *(a + 1))
		{
			*b++ = *a;
		}
		else
		{
			a += 2;
			b = *(c - 1) + 1; // d contains a \ already

			// the replaced \ is the last one 
			*c = NULL;
			c--;
		}
	}
	*b = 0;
	return path;
}

bool GetIniString(char const* szSection, char const* szKey, char const* szDefault, char* s, VWB_uint sz, char const* szConfigFile)
{
	bool bRet = false;
	FILE* f = NULL;
	bool bInChannel = false;
	if (szDefault)
		strcpy_s(s, sz, szDefault);
	else
		*s = 0;

	if (NO_ERROR == fopen_s(&f, szConfigFile, "r"))
	{
		for (char line[32678]; NULL != fgets(line, 32677, f); )
		{
			//// remove comments
			//for( wchar_t* c = line; *c; c++ )
			//	if( L';' == *c )
			//	{
			//		*c = 0;
			//		break;
			//	}
			// remove white spaces at front and end
			char* b = line;
			char* e = b;
			for (; *b; b++)
				if (' ' != *b && '\t' != *b)
					break;
			for (char* c = b; *c; c++)
				if (' ' != *c && '\t' != *c && '\n' != *c)
					e = c;
			*(e + 1) = 0;

			// match section
			if (!bInChannel)
			{
				// match section
				if ('[' == *b)
				{
					b++;
					e = strchr(b, ']');
					if (e)
					{
						*e = 0;
						if (0 == _stricmp(szSection, b))
							bInChannel = true;
					}
				}
			}
			else // match key
			{
				if ('[' == *b)
				{
					bInChannel = false;
				}
				else
				{
					char* bv = strchr(b, '=');
					if (bv)
					{
						*bv++ = 0;
						if (0 == _stricmp(b, szKey))
						{
							if (NO_ERROR == strcpy_s(s, sz, bv))
							{
								bRet = true;
								break;
							}
						}
					}
				}
			}
		}
		fclose(f);
	}
	return bRet;
}

VWB_int GetIniInt(char const* szSection, char const* szKey, VWB_int iDefault, char const* szConfigFile)
{
	char s[32678] = { 0 };
	if (GetIniString(szSection, szKey, "", s, 32678, szConfigFile))
		return atoi(s);
	return iDefault;
}

VWB_float GetIniFloat(char const* szSection, char const* szKey, VWB_float fDefault, char const* szConfigFile)
{
	char s[32678] = { 0 };
	if (GetIniString(szSection, szKey, "", s, 32678, szConfigFile))
		return (VWB_float)atof(s);
	return fDefault;
}

VWB_float* GetIniMat(char const* szSection, char const* szKey, int dimX, int dimY, VWB_float const* fDefault, VWB_float* f, char const* szConfigFile)
{
	if (NULL == f)
		return NULL;
	if (0 == dimX || 0 == dimY)
		return NULL;
	char s[32678] = { 0 };
	GetIniString(szSection, szKey, "", s, 32678, szConfigFile);
	char* pS = s;
	if( *pS != '[' )
		return NULL;
	pS++;
	int o = 0;
	int oE = dimX * dimY;
	for (; 1 == sscanf_s(pS, "%f", &f[o]) && o != oE; o++)
	{
		char* pFS;
		if ( (pFS = strchr( pS, ',' )) || ( pFS = strchr( pS, ';' ) ) || ( pFS = strchr( pS, ']' ) ) )
			pS = pFS + 1;
		else
			break;
	}
	if (oE == o)
		return f;
	if (NULL != fDefault)
	{
		memcpy(f, fDefault, sizeof(VWB_float) * dimX * dimY);
		return f;
	}
	for (int y = 0; y != dimY; y++)
		for (int x = 0; x != dimX; x++)
			f[dimX * y + x] = 0.0f;
	return NULL;
}
