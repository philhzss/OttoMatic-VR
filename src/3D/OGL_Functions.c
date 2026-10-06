#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "game.h"

PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB			= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB	= NULL;

PFNGLGENBUFFERSPROC					procptr_glGenBuffers				= NULL;
PFNGLBINDBUFFERPROC					procptr_glBindBuffer				= NULL;
PFNGLBUFFERDATAPROC					procptr_glBufferData				= NULL;
PFNGLDELETEBUFFERSPROC				procptr_glDeleteBuffers				= NULL;

void OGL_InitFunctions(void)
{
	procptr_glActiveTextureARB			= (PFNGLACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glActiveTextureARB");
	procptr_glClientActiveTextureARB	= (PFNGLCLIENTACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glClientActiveTextureARB");

	GAME_ASSERT(procptr_glActiveTextureARB);
	GAME_ASSERT(procptr_glClientActiveTextureARB);

	procptr_glGenBuffers				= (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffers");
	procptr_glBindBuffer				= (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBuffer");
	procptr_glBufferData				= (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferData");
	procptr_glDeleteBuffers				= (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffers");

	GAME_ASSERT(procptr_glGenBuffers);
	GAME_ASSERT(procptr_glBindBuffer);
	GAME_ASSERT(procptr_glBufferData);
	GAME_ASSERT(procptr_glDeleteBuffers);
}
