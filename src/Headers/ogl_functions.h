#pragma once

#include <SDL3/SDL_opengl.h>

extern PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB;

#define glActiveTextureARB					procptr_glActiveTextureARB
#define glClientActiveTextureARB			procptr_glClientActiveTextureARB

extern PFNGLGENBUFFERSPROC					procptr_glGenBuffers;
extern PFNGLBINDBUFFERPROC					procptr_glBindBuffer;
extern PFNGLBUFFERDATAPROC					procptr_glBufferData;
extern PFNGLDELETEBUFFERSPROC				procptr_glDeleteBuffers;

#define glGenBuffers						procptr_glGenBuffers
#define glBindBuffer						procptr_glBindBuffer
#define glBufferData						procptr_glBufferData
#define glDeleteBuffers						procptr_glDeleteBuffers

void OGL_InitFunctions(void);
