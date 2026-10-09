/****************************/
/*   OGL_STATECACHE.C	    */
/****************************/

// OGL_PushState used to read the current GL state back from the driver with
// glIsEnabled/glGet*. Each of those forces the game thread to wait until NVIDIA's
// "Threaded optimization" driver thread has caught up, and PushState runs for every
// model drawn (MO_DrawGroup), per eye -- thousands of stalls per frame.
//
// Instead, we keep our own copy of the state PushState saves. ogl_support.h redirects
// glEnable/glDisable/glBlendFunc/glDepthMask/glColor* to the wrappers below, which
// update the copy and then call the real GL function.
//
// This file defines OGL_STATECACHE_IMPL so it sees the real GL functions, not the redirects.

#define OGL_STATECACHE_IMPL
#include "game.h"

/****************************/
/*    VARIABLES             */
/****************************/

OGLStateCache gGLState;


/******************** GET CACHED FLAG *********************/
//
// Returns the cached on/off flag for a capability we track, or NULL for ones we don't
// (GL_LIGHTING, GL_ALPHA_TEST, etc. are passed straight through).
// GL_TEXTURE_2D has one flag per texture unit, so we return the active unit's.
//

static Boolean* GetCachedFlag(GLenum cap)
{
	switch (cap)
	{
		case GL_CULL_FACE:		return &gGLState.cullFace;
		case GL_DEPTH_TEST:		return &gGLState.depthTest;
		case GL_NORMALIZE:		return &gGLState.normalize;
		case GL_TEXTURE_2D:		return &gGLState.texture2D[gGLState.activeTextureUnit];
		case GL_FOG:			return &gGLState.fog;
		case GL_BLEND:			return &gGLState.blend;
		default:				return NULL;
	}
}


/******************** INIT *********************/
//
// Reads the real state from the driver once, right after the context is made current.
// A handful of queries at startup cost nothing; it's doing it per draw that hurts.
//

void OGL_StateCache_Init(void)
{
	GLint activeTexture = GL_TEXTURE0_ARB;
	glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
	gGLState.activeTextureUnit = activeTexture - GL_TEXTURE0_ARB;
	if (gGLState.activeTextureUnit < 0 || gGLState.activeTextureUnit >= OGL_STATECACHE_MAX_TEXTURE_UNITS)
		gGLState.activeTextureUnit = 0;

	for (int unit = 0; unit < OGL_STATECACHE_MAX_TEXTURE_UNITS; unit++)		// GL default: texturing off on every unit
		gGLState.texture2D[unit] = false;
	gGLState.texture2D[gGLState.activeTextureUnit] = glIsEnabled(GL_TEXTURE_2D);

	gGLState.cullFace	= glIsEnabled(GL_CULL_FACE);
	gGLState.depthTest	= glIsEnabled(GL_DEPTH_TEST);
	gGLState.normalize	= glIsEnabled(GL_NORMALIZE);
	gGLState.fog		= glIsEnabled(GL_FOG);
	gGLState.blend		= glIsEnabled(GL_BLEND);

	glGetIntegerv(GL_BLEND_SRC, &gGLState.blendSrc);
	glGetIntegerv(GL_BLEND_DST, &gGLState.blendDst);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &gGLState.depthMask);
	glGetFloatv(GL_CURRENT_COLOR, gGLState.color);
}


/******************** VERIFY (DEBUG ONLY) *********************/
//
// Compares the cache against the driver and reports any difference, which would mean
// some code changed state without going through the wrappers. Then resyncs so a single
// mismatch isn't reported forever.
//
// The current color is not checked: after drawing with a color array (per-vertex colors),
// GL leaves the current color undefined, so it would report false mismatches.
//

#ifdef _DEBUG
void OGL_StateCache_Verify(void)
{
	static const struct { GLenum cap; const char* name; } kCaps[] =
	{
		{ GL_CULL_FACE,		"GL_CULL_FACE" },
		{ GL_DEPTH_TEST,	"GL_DEPTH_TEST" },
		{ GL_NORMALIZE,		"GL_NORMALIZE" },
		{ GL_TEXTURE_2D,	"GL_TEXTURE_2D" },
		{ GL_FOG,			"GL_FOG" },
		{ GL_BLEND,			"GL_BLEND" },
	};

	for (int i = 0; i < (int)(sizeof(kCaps) / sizeof(kCaps[0])); i++)
	{
		Boolean* cached = GetCachedFlag(kCaps[i].cap);
		Boolean real = glIsEnabled(kCaps[i].cap);
		if (*cached != real)
		{
			printf("GL state cache mismatch: %s cached=%d real=%d\n", kCaps[i].name, *cached, real);
			*cached = real;
		}
	}

	GLint realSrc, realDst;
	GLboolean realDepthMask;
	glGetIntegerv(GL_BLEND_SRC, &realSrc);
	glGetIntegerv(GL_BLEND_DST, &realDst);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &realDepthMask);

	if (realSrc != gGLState.blendSrc || realDst != gGLState.blendDst)
	{
		printf("GL state cache mismatch: blend func cached=0x%X/0x%X real=0x%X/0x%X\n",
				gGLState.blendSrc, gGLState.blendDst, realSrc, realDst);
		gGLState.blendSrc = realSrc;
		gGLState.blendDst = realDst;
	}

	if (realDepthMask != gGLState.depthMask)
	{
		printf("GL state cache mismatch: depth mask cached=%d real=%d\n", gGLState.depthMask, realDepthMask);
		gGLState.depthMask = realDepthMask;
	}
}
#endif


#pragma mark -

/******************** WRAPPERS *********************/
//
// Update the cache, then call the real GL function.
//

void OGL_Cached_glEnable(GLenum cap)
{
	Boolean* flag = GetCachedFlag(cap);
	if (flag)
		*flag = true;
	glEnable(cap);
}

void OGL_Cached_glDisable(GLenum cap)
{
	Boolean* flag = GetCachedFlag(cap);
	if (flag)
		*flag = false;
	glDisable(cap);
}

void OGL_Cached_glActiveTextureARB(GLenum texture)
{
	int unit = (int)(texture - GL_TEXTURE0_ARB);
	if (unit >= 0 && unit < OGL_STATECACHE_MAX_TEXTURE_UNITS)
		gGLState.activeTextureUnit = unit;
	glActiveTextureARB(texture);
}

void OGL_Cached_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	gGLState.blendSrc = sfactor;
	gGLState.blendDst = dfactor;
	glBlendFunc(sfactor, dfactor);
}

void OGL_Cached_glDepthMask(GLboolean flag)
{
	gGLState.depthMask = flag;
	glDepthMask(flag);
}

void OGL_Cached_glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	gGLState.color[0] = r;
	gGLState.color[1] = g;
	gGLState.color[2] = b;
	gGLState.color[3] = 1.0f;								// glColor3f sets alpha to 1
	glColor3f(r, g, b);
}

void OGL_Cached_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	gGLState.color[0] = r;
	gGLState.color[1] = g;
	gGLState.color[2] = b;
	gGLState.color[3] = a;
	glColor4f(r, g, b, a);
}

void OGL_Cached_glColor4fv(const GLfloat* v)
{
	gGLState.color[0] = v[0];
	gGLState.color[1] = v[1];
	gGLState.color[2] = v[2];
	gGLState.color[3] = v[3];
	glColor4fv(v);
}
