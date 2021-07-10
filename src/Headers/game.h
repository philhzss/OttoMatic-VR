#pragma once

		/* MY BUILD OPTIONS */

#define __LITTLE_ENDIAN__	1


		/* HEADERS */

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#if __APPLE__
	#include <OpenGL/glu.h>		// gluPerspective
#else
	#include <GL/glu.h>			// gluPerspective
#endif

#include "Pomme.h"

#include "globals.h"
#include "structs.h"


#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "mobjtypes.h"
#include "misc.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "sparkle.h"
#include "bg3d.h"
#include "camera.h"
#include "collision.h"
#include 	"input.h"
#include "file.h"
#include "window.h"
#include "player.h"
#include "terrain.h"
#include "humans.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include	"infobar.h"
#include "triggers.h"
#include "effects.h"
#include "shards.h"
#include "bones.h"
#include "vaportrails.h"
#include "splineitems.h"
#include "mytraps.h"
#include "enemy.h"
#include "items.h"
#include "sky.h"
#include "water.h"
#include "fences.h"
#include "miscscreens.h"
#include "objects.h"
#include "mainmenu.h"
#include "lzss.h"
#include "3dmath.h"
#include "ogl_functions.h"
#include "localization.h"
#include "textmesh.h"
#include "tga.h"


#define GAME_ASSERT(condition) do { if (!(condition)) DoAssert(#condition, __func__, __LINE__); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoAssert(message, __func__, __LINE__); } while(0)
#define SOURCE_PORT_PLACEHOLDER() DoAssert("Source port: this function is not implemented yet", __func__, __LINE__)
#define SOURCE_PORT_MINOR_PLACEHOLDER() printf("Source port: TODO: %s:%d\n", __func__, __LINE__)
