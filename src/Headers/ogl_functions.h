#pragma once

#include <SDL3/SDL_opengl.h>

extern PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB;

// glActiveTextureARB goes through the GL state cache (OGL_StateCache.c), because
// GL_TEXTURE_2D is enabled/disabled per texture unit and the cache must know which unit is active.
void OGL_Cached_glActiveTextureARB(GLenum texture);

#ifndef OGL_STATECACHE_IMPL
	#define glActiveTextureARB				OGL_Cached_glActiveTextureARB
#else
	#define glActiveTextureARB				procptr_glActiveTextureARB
#endif
#define glClientActiveTextureARB			procptr_glClientActiveTextureARB

void OGL_InitFunctions(void);
