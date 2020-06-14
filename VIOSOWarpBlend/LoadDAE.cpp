#include "Platform.h"

#include "LoadDAE.h"
#include <3rdparty/tinyxml2/tinyxml2.h>
#include "logging.h"
#include "PathHelper.h"
#include "Platform.h"

#include <cstring>

using namespace tinyxml2;

const TriangleMesh::MeshVertex TriangleMesh::_InvVertex = { VWB_VEC3f(NAN,0,0), VWB_VEC3f(NAN,0,0), {NAN, 0} };

typedef enum
{
	VERTEXBUFFER_SLOT_UNUSED = 0,
	VERTEXBUFFER_SLOT_POS_X ,
	VERTEXBUFFER_SLOT_POS_Y ,
	VERTEXBUFFER_SLOT_POS_Z ,
	VERTEXBUFFER_SLOT_POS_W ,
	VERTEXBUFFER_SLOT_NORM_X,
	VERTEXBUFFER_SLOT_NORM_Y,
	VERTEXBUFFER_SLOT_NORM_Z,
	VERTEXBUFFER_SLOT_NORM_W,
	VERTEXBUFFER_SLOT_TANG_X,
	VERTEXBUFFER_SLOT_TANG_Y,
	VERTEXBUFFER_SLOT_TANG_Z,
	VERTEXBUFFER_SLOT_TANG_W,
	VERTEXBUFFER_SLOT_TEX_S ,
	VERTEXBUFFER_SLOT_TEX_T ,
	VERTEXBUFFER_SLOT_TEX_U ,
	VERTEXBUFFER_SLOT_TEX_V ,
	VERTEXBUFFER_SLOT_COL_R ,
	VERTEXBUFFER_SLOT_COL_G ,
	VERTEXBUFFER_SLOT_COL_B ,
	VERTEXBUFFER_SLOT_COL_A ,
	VERTEXBUFFER_SLOT_MAX ,
} VERTEXBUFFER_SLOT;

const int AssignSlot[VERTEXBUFFER_SLOT_MAX] =
{-1,0,1,2,-1,3,4,5,-1,-1,-1,-1,-1,6,7,-1,-1,-1,-1,-1,-1};

typedef enum
{
	SEMANTIC_POSITION,
	SEMANTIC_COLOR,
	SEMANTIC_TEXCOORD,
	SEMANTIC_TANGENT,
	SEMANTIC_NORMAL,
} SEMANTIC;

typedef struct DAESrcInfo
{
	int count;
	int stride;
	VERTEXBUFFER_SLOT slots[16];
	TriangleMesh::FloatVec source;
} DAESrcInfo;

DAESrcInfo getSrcInfo( const char* id, XMLElement* pMesh, SEMANTIC sem )
{
	DAESrcInfo inf = {0,0,{VERTEXBUFFER_SLOT_UNUSED},TriangleMesh::FloatVec() };

	XMLElement* pSource = pMesh->FirstChildElement( "source" );
	while( pSource )
	{
		char const* sz = pSource->Attribute( "id" );
		if( NULL != sz && 0 == strcmp( sz, id ) )
		{
			XMLElement* pSN = pSource->FirstChildElement( "technique_common" );
			if( pSN )
			{
				pSN = pSN->FirstChildElement( "accessor" );
				if( pSN )
				{
					inf.count = pSN->IntAttribute( "count" );
					inf.stride = pSN->IntAttribute( "stride" );
					XMLElement* pStream = pSource->FirstChildElement( "float_array" );
					if( pStream )
					{
						const char* szStream = pStream->GetText();
						inf.source.reserve( inf.count * inf.stride );
						VWB_float f;
						for( char const* sz = szStream; *sz; sz++)
						{
							if( ' ' == *sz ||
								'\t' == *sz ||
								'\r' == *sz ||
								'\n' == *sz )
							{
								if( 1 == sscanf_s( szStream, "%f", &f ) )
									inf.source.push_back(f);
								szStream = sz + 1;
							}
						}
						if( 1 == sscanf_s( szStream, "%f", &f ) )
							inf.source.push_back(f);
					}
					else
						break;

					int iSlot = 0;
					for( pSN = pSN->FirstChildElement( "param" ); NULL != pSN && iSlot != 16; pSN = pSN->NextSiblingElement("param"), iSlot++ )
					{
						const char* name = pSN->Attribute( "name" );
						if( name )
						{
							int tSL;
							if( 0 == strcmp( "X", name ) )
								tSL = 0;
							else if( 0 == _stricmp( "Y", name ) )
								tSL = 1;
							else if( 0 == _stricmp( "Z", name ) )
								tSL = 2;
							else if( 0 == _stricmp( "W", name ) )
								tSL = 3;
							else if( 0 == _stricmp( "R", name ) )
								tSL = 0;
							else if( 0 == _stricmp( "G", name ) )
								tSL = 1;
							else if( 0 == _stricmp( "B", name ) )
								tSL = 2;
							else if( 0 == _stricmp( "B", name ) )
								tSL = 3;
							else if( 0 == _stricmp( "S", name ) )
								tSL = 0;
							else if( 0 == _stricmp( "T", name ) )
								tSL = 1;
							else if( 0 == _stricmp( "U", name ) )
								tSL = 2;
							else if( 0 == _stricmp( "V", name ) )
								tSL = 3;
							else
								continue;

							switch( sem )
							{
							case SEMANTIC_COLOR:
								inf.slots[iSlot] = VERTEXBUFFER_SLOT(VERTEXBUFFER_SLOT_COL_R + tSL);
								break;
							case SEMANTIC_POSITION:
								inf.slots[iSlot] = VERTEXBUFFER_SLOT(VERTEXBUFFER_SLOT_POS_X + tSL);
								break;
							case SEMANTIC_NORMAL:
								inf.slots[iSlot] = VERTEXBUFFER_SLOT(VERTEXBUFFER_SLOT_NORM_X + tSL);
								break;
							case SEMANTIC_TEXCOORD:
								inf.slots[iSlot] = VERTEXBUFFER_SLOT(VERTEXBUFFER_SLOT_TEX_S + tSL);
								break;
							case SEMANTIC_TANGENT:
								inf.slots[iSlot] = VERTEXBUFFER_SLOT(VERTEXBUFFER_SLOT_TANG_X + tSL);
								break;
							default:
								continue;
							}
						}
					}
				}
			}
			break;
		}
		pSource = pSource->NextSiblingElement( "source" );
	}

	return inf;
}

VWB_ERROR loadDAE( Scene& scene, char const* path )
{
	if( NULL == path || 0 == *path )
	{
		logStr( 0, "ERROR: loadDAE: Missing path to geometry file.\n" );
		return VWB_ERROR_PARAMETER;
	}
	VWB_ERROR ret = VWB_ERROR_NONE;
	char const* pP = path;
	char pp[MAX_PATH];
	char const* pP2;
	do
	{
		pP2 = strchr( pP, ',' );
#ifdef WIN32
		if( pP2 )
			strncpy_s( pp, pP, pP2 - pP );
		else
			strcpy_s( pp, MAX_PATH, pP );
#else
		if( pP2 )
			strncpy( pp, pP, pP2 - pP );
		else
			strcpy( pp, pP);
#endif
		MkPath( pp, MAX_PATH, ".dae" );

		logStr( 2, "INFO: loadDAE: Open \"%s\"...\n", pp );
		XMLDocument doc;
		if( XML_SUCCESS == doc.LoadFile( path ) )
		{
			logStr( 2, "INFO: DAELoad: File loaded.\n" );
			XMLElement* pRoot = doc.FirstChildElement("COLLADA");
			if( pRoot )
			{
				VWB_float scale = 1;
				XMLElement* pNode = NULL;
				XMLAttribute* pAttr = NULL;
				pNode = pRoot->FirstChildElement( "asset" );
				if( pNode )
				{
					pNode = pNode->FirstChildElement("unit");
					if( pNode )
					{
						scale = pNode->FloatAttribute( "meter" );
						if( 0 == scale )
							scale = 1;
						logStr( 2, "INFO: DAELoad: set scale to %f.\n", scale );
					}
				}
				pNode = pRoot->FirstChildElement("library_geometries");
				if( pNode )
				{
					pNode = pNode->FirstChildElement( "geometry" );
					if( pNode )
					{
						while( pNode )
						{
							static const Geometry g;
							scene.push_back( g );
							Geometry& geom = scene.back();
							
							XMLElement* pMesh = pNode->FirstChildElement( "mesh" );
							while( pMesh )
							{
								static const TriangleMesh m;
								geom.push_back(m);
								TriangleMesh& mesh = geom.back();
								char const* szPositions = NULL;
								int iPositionOffset = 0;
								char const* szVertices = NULL;
								int iVerticesOffset = 0;
								char const* szTexcoord = NULL;
								int iTexcoordOffset = 0;
								char const* szNormals = NULL;
								int iNormalsOffset = 0;
								char const* szIndices = NULL;
								int iIndices = 0;
								int iMaxOffset = 0;

								XMLElement* pSN = pMesh->FirstChildElement( "triangles" );
								XMLElement* pSSN;
								while( pSN )
								{
									pSSN = pSN->FirstChildElement("p");
									szVertices = NULL;
									szPositions = NULL;
									szNormals = NULL;
									szTexcoord = NULL;
									if( NULL != pSSN )
										szIndices = pSSN->GetText();
									iIndices = pSN->IntAttribute("count");
									pSSN = pSN->FirstChildElement( "input" );
									while( pSSN )
									{
										char const* sz = pSSN->Attribute("semantic" );
										if( NULL != sz )
										{ 
											const int iOffs = pSSN->IntAttribute( "offset" );
											if( iOffs > iMaxOffset )
												iMaxOffset = iOffs;

											if( 0 == strcmp( sz, "VERTEX" ) )
											{
												szVertices = pSSN->Attribute( "source" );
												if( szVertices )
													szVertices++;
												iVerticesOffset = iOffs;
											}
											else if( 0 == strcmp( sz, "POSITION" ) )
											{
												szPositions = pSSN->Attribute( "source" );
												if( szPositions )
													szPositions++;
												iPositionOffset = iOffs;
											}
											else if( 0 == strcmp( sz, "NORMAL" ) )
											{
												szNormals = pSSN->Attribute( "source" );
												if( szNormals )
													szNormals++;
												iNormalsOffset = iOffs;
											}
											else if( 0 == strcmp( sz, "TEXCOORD" ) )
											{
												const char* set = pSSN->Attribute( "set" );
												if( NULL == set || 0 == strcmp( "0", set ) )
												{
													szTexcoord = pSSN->Attribute( "source" );
													if( szTexcoord )
														szTexcoord++;
													iTexcoordOffset = iOffs;
												}
											}
										}
										pSSN = pSSN->NextSiblingElement( "input" );
									}
									if( ( NULL != szVertices || NULL != szPositions ) && NULL != szTexcoord )
										break;

									pSN = pSN->NextSiblingElement( "triangles" );
								}

								if( NULL == szPositions && NULL != szVertices ) // translate vertex into pos and normal
								{
									pSN = pMesh->FirstChildElement( "vertices" );
									while( pSN )
									{
										char const* sz = pSN->Attribute( "id" );
										if( 0 == strcmp( sz, szVertices ) )
										{
											pSSN = pSN->FirstChildElement( "input" );
											while( pSSN )
											{
												char const* sz = pSSN->Attribute("semantic" );
												if( NULL != sz )
												{ 
													if( 0 == strcmp( sz, "POSITION" ) )
													{
														szPositions = pSSN->Attribute( "source" );
														if( szPositions )
															szPositions++;
														iPositionOffset = iVerticesOffset;
													}
													else if( 0 == strcmp( sz, "NORMAL" ) )
													{
														szNormals = pSSN->Attribute( "source" );
														if( szNormals )
															szNormals++;
														iNormalsOffset = iVerticesOffset;
													}
													else if( 0 == strcmp( sz, "TEXCOORD" ) )
													{
														szTexcoord = pSSN->Attribute( "source" );
														if( szTexcoord )
															szTexcoord++;
														iTexcoordOffset = iVerticesOffset;
													}
												}
												pSSN = pSSN->NextSiblingElement( "input" );
											}
											break;
										}
										pSN = pSN->NextSiblingElement( "vertices" );
									}
								}

								if( NULL != szPositions && NULL != szIndices )
								{
									// read indices
									int i = 0;
									iMaxOffset++;
									TriangleMesh::IntVec indVec;
									indVec.reserve( iIndices * 3 );
									for( char const* sz = szIndices; *sz; sz++)
									{
										if( ' ' == *sz ||
											'\t' == *sz ||
											'\r' == *sz ||
											'\n' == *sz )
										{
											if( 1 == sscanf_s( szIndices, "%i", &i ) )
												indVec.push_back(i);
											szIndices = sz + 1;
										}
									}
									if( 1 == sscanf_s( szIndices, "%i", &i ) )
										indVec.push_back(i);

									// gather streams#
									// do positions first
									const int sz = (int)indVec.size();
									DAESrcInfo inf = getSrcInfo( szPositions, pMesh, SEMANTIC_POSITION );
									if( inf.count && inf.stride && !inf.source.empty() )
									{
										mesh.m_vertices.resize( inf.count, TriangleMesh::_InvVertex );
										mesh.m_indeces.resize( indVec.size() / iMaxOffset, -1 );
										for( int iPos = iPositionOffset, iInd = 0; iPos < sz; iPos+= iMaxOffset, iInd++ )
										{
											VWB_int index = indVec[iPos];
											mesh.m_indeces[iInd]= index;
											for( int i = 0; i != inf.stride; i++ )
											{
												int j = AssignSlot[inf.slots[i]];
												if( 0<=j && j < 8 )
													(&mesh.m_vertices[index].pos.x)[j] = inf.source[index*inf.stride+i];
											}
										}
										mesh.m_validFlags|= TriangleMesh::VALIDFLAG_POS;
									}
									else
									{
										logStr( 1, "WARNING: loadDAE: position empty.\n" );
										ret = VWB_ERROR_VWF_LOAD;
									}

									// do texture next
									if( NULL != szTexcoord )
									{
										DAESrcInfo inf = getSrcInfo( szTexcoord, pMesh, SEMANTIC_TEXCOORD );
										if( inf.count && inf.stride && !inf.source.empty() )
										{
											for( int iTex = iTexcoordOffset, iInd = 0; iTex < sz; iTex+= iMaxOffset, iInd++ )
											{
												VWB_int indexS = indVec[iTex];
												VWB_int indexV = mesh.m_indeces[iInd];
												for( int i = 0; i != inf.stride; i++ )
												{
													int j = AssignSlot[inf.slots[i]];
													if( 0<=j && j < 8 )
														(&mesh.m_vertices[indexV].pos.x)[j] = inf.source[indexS*inf.stride+i];
												}
											}
											mesh.m_validFlags|= TriangleMesh::VALIDFLAG_TEX;
										}
										else
										{
											logStr( 1, "WARNING: loadDAE: texcoord empty.\n" );
											ret = VWB_ERROR_VWF_LOAD;
										}
									}
									//// do normals last
									if( NULL != szNormals )
									{
										DAESrcInfo inf = getSrcInfo( szNormals, pMesh, SEMANTIC_NORMAL );
										if( inf.count && inf.stride && !inf.source.empty() )
										{
											for( int iNorm = iNormalsOffset, iInd = 0; iNorm < sz; iNorm+= iMaxOffset, iInd++ )
											{
												VWB_int indexS = indVec[iNorm];
												VWB_int indexV = mesh.m_indeces[iInd];
												for( int i = 0; i != inf.stride; i++ )
												{
													int j = AssignSlot[inf.slots[i]];
													if( 0<=j && j < 8 )
														(&mesh.m_vertices[indexV].pos.x)[j] = inf.source[indexS*inf.stride+i];
												}
											}
											mesh.m_validFlags|= TriangleMesh::VALIDFLAG_NORM;
										}
										else 
										{
											logStr( 1, "WARNING: loadDAE: normals empty.\n" );
											ret = VWB_ERROR_VWF_LOAD;
										}
									}
									else
									{
										for( TriangleMesh::MeshVertices::iterator it = mesh.m_vertices.begin(); it != mesh.m_vertices.end(); it++ )
										{
											it->norm.x = it->norm.y = it->norm.z = 0;
										}
										for( TriangleMesh::IntVec::iterator it = mesh.m_indeces.begin(); it != mesh.m_indeces.end(); )
										{
											VWB_VEC3f& n = mesh.m_vertices[*it].norm;
											const VWB_VEC3f p1 = mesh.m_vertices[*it++].pos;
											const VWB_VEC3f p2 = mesh.m_vertices[*it++].pos;
											const VWB_VEC3f p3 = mesh.m_vertices[*it++].pos;
											//todo: if quite same length but different direction, split up vertices to form edge
											n+= ( p2 - p1 ) * ( p3 - p1 );
										}
										mesh.m_validFlags|= TriangleMesh::VALIDFLAG_NORM;
									}
								}
								else
								{
									logStr( 1, "WARNING: loadDAE: empty node.\n" );
									ret = VWB_ERROR_VWF_LOAD;
								}

								// tristrips
								// trifans
								// polygons
								// polylist

								pMesh = pMesh->NextSiblingElement( "mesh" );
							}

							pNode = pNode->NextSiblingElement("geometry");
							logStr( 2, "INFO: loadDAE: geometry with %u meshes added.\n", (VWB_uint)geom.size() );
						}
					}
					else
					{
						logStr( 0, "ERROR: loadDAE: geometry node not found.\n" );
						ret = VWB_ERROR_VWF_LOAD;
					}

				}
				else
				{
					logStr( 0, "ERROR: loadDAE: library_geometries node not found.\n" );
					ret = VWB_ERROR_VWF_LOAD;
				}

			}
			else
			{
				logStr( 0, "ERROR: loadDAE: COLLADA node not found.\n" );
				ret = VWB_ERROR_VWF_LOAD;
			}
		}
		else
		{
			ret = VWB_ERROR_VWF_LOAD;
			logStr( 0, "ERROR: loadDAE: Failed to open.\n" );
		}
	} while(pP2);

	return ret;
}

