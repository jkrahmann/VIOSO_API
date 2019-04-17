#ifndef VWB_GLWARPBLEND_HPP
#define VWB_GLWARPBLEND_HPP

#include "../common.h"
#include "GLext.h"
#include <string>

class GLWarpBlend : public VWB_Warper_base
{
public:

protected:
    //HGLRC					m_context;          // the gl render context
    //HDC						m_hDC;				// the device context
    GLuint				    m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
    GLuint				    m_texBlend;         // the blend lookup texture
	GLuint					m_texBB;			// the back buffer copy, if demanded

	GLuint					m_locWarp;
	GLuint					m_locBorder;
	GLuint					m_locBlend;
	GLuint					m_locSize;
	GLuint					m_locDoNotBlend;
	GLuint					m_locOffsScale;
	GLuint					m_locContent;
	GLuint					m_locContentBypass;
	GLuint					m_locMatView;
	GLuint					m_locDim;
	GLuint					m_locSmooth;
	GLuint					m_locParams;
	GLuint					m_iVertexArray;
    GLuint					m_matrix;

	GLuint				    m_VertexShader;
    GLuint				    m_FragmentShader;
    GLuint				    m_FragmentShaderBypass;
    GLuint				    m_Program;
    GLuint				    m_ProgramBypass;

#ifdef _DEBUG
	VWB_WarpRecord			m_vTests[5];
#endif

public:
 	GLWarpBlend();

	virtual ~GLWarpBlend(void);

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

    virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  


protected:
	// Helper functions
    GLuint  CreatePixelShader(const std::string &src, const std::string &main, const std::string &profile) const;
    std::string             PixelShaderSource() const;
    void                    SetTexture(GLuint texLoc, GLuint tex, GLuint wrapMode = GL_CLAMP_TO_BORDER) const;
};

#endif //ndef VWB_GLWARPBLEND_HPP
