/****************************/
/*   	CAMERA.C    	    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCamera_Saucer(ObjNode *saucer);
static void InitCamera_Saucer(ObjNode *saucer);

static void ResetCameraSettings(void);

/****************************/
/*    CONSTANTS             */
/****************************/


#define	CAM_MINY			120.0f

#define	CAMERA_CLOSEST		150.0f
#define	CAMERA_FARTHEST		800.0f

#define	NUM_FLARE_TYPES		4
#define	NUM_FLARES			6


/*********************/
/*    VARIABLES      */
/*********************/

static OGLCameraPlacement	gAnaglyphCameraBackup;		// backup of original camera info before offsets applied

Boolean				gAutoRotateCamera = false;
float				gAutoRotateCameraSpeed = 0;

Boolean				gDrawLensFlare = true, gFreezeCameraFromXZ = false, gFreezeCameraFromY = false;
Boolean				gForceCameraAlignment = false;

float				gCameraUserRotY = 0;
float				mouseCameraAngleY;
float				gPlayerToCameraAngle = 0.0f;
static float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
float		gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;
float		gMinHeightOffGround;

static OGLPoint3D	gSunCoord;

static const float	gFlareOffsetTable[]=
{
	1.0,
	.6,
	.3,
	1.0/8.0,
	-.25,
	-.5
};


static const float	gFlareScaleTable[]=
{
	.3,
	.06,
	.1,
	.2,
	.03,
	.1
};

static const Byte	gFlareImageTable[]=
{
	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare1
};




/*********************** DRAW LENS FLARE ***************************/

void DrawLensFlare(void)
{
short			i;
float			x,y,dot;
OGLPoint3D		sunScreenCoord,from;
float			cx,cy;
float			dx,dy,length;
OGLVector3D		axis,lookAtVector,sunVector;
static OGLColorRGBA	transColor = {1,1,1,1};
int				px,py,pw,ph;

	if (!gDrawLensFlare)
		return;


			/************/
			/* SET TAGS */
			/************/

	OGL_PushState();

	OGL_DisableLighting();												// Turn OFF lighting
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	SetColor4f(1,1,1,1);										// full white & alpha to start with


			/* CALC SUN COORD */

	from = gGameViewInfoPtr->cameraPlacement.cameraLocation;
	gSunCoord.x = from.x - (gWorldSunDirection.x * gGameViewInfoPtr->yon);
	gSunCoord.y = from.y - (gWorldSunDirection.y * gGameViewInfoPtr->yon);
	gSunCoord.z = from.z - (gWorldSunDirection.z * gGameViewInfoPtr->yon);



	/* CALC DOT PRODUCT BETWEEN VIEW AND LIGHT VECTORS TO SEE IF OUT OF RANGE */

	FastNormalizeVector(from.x - gSunCoord.x,
						from.y - gSunCoord.y,
						from.z - gSunCoord.z,
						&sunVector);

	FastNormalizeVector(gGameViewInfoPtr->cameraPlacement.pointOfInterest.x - from.x,
						gGameViewInfoPtr->cameraPlacement.pointOfInterest.y - from.y,
						gGameViewInfoPtr->cameraPlacement.pointOfInterest.z - from.z,
						&lookAtVector);

	dot = OGLVector3D_Dot(&lookAtVector, &sunVector);
	if (dot >= 0.0f)
		goto bye;

	dot = acos(dot) * -2.0f;				// get angle & modify it
	transColor.a = cos(dot);				// get cos of modified angle


			/* CALC SCREEN COORDINATE OF LIGHT */

	OGLPoint3D_Transform(&gSunCoord, &gWorldToWindowMatrix, &sunScreenCoord);


			/* CALC CENTER OF VIEWPORT */

	OGL_GetCurrentViewport(&px, &py, &pw, &ph);
	cx = pw/2 + px;
	cy = ph/2 + py;


			/* CALC VECTOR FROM CENTER TO LIGHT */

	dx = sunScreenCoord.x - cx;
	dy = sunScreenCoord.y - cy;
	length = sqrt(dx*dx + dy*dy);
	FastNormalizeVector(dx, dy, 0, &axis);


			/***************/
			/* DRAW FLARES */
			/***************/

			/* INIT MATRICES */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	for (i = 0; i < NUM_FLARES; i++)
	{
		float	sx,sy,o,fx,fy;

		if (i == 0)
			gGlobalTransparency = .99;		// sun is always full brightness (leave @ < 1 to ensure GL_BLEND)
		else
			gGlobalTransparency = transColor.a;

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gFlareImageTable[i]].materialObject);		// activate material



		if (i == 1)												// always draw sun, but fade flares based on dot
		{
			if (transColor.a <= 0.0f)							// see if faded all out
				break;
			SetColor4fv(&transColor.r);
		}


		o = gFlareOffsetTable[i];
		sx = gFlareScaleTable[i];
		sy = sx * gCurrentAspectRatio;

		x = cx + axis.x * length * o;
		y = cy + axis.y * length * o;

		fx = x / (pw/2) - 1.0f;
		fy = (ph-y) / (ph/2) - 1.0f;

		glBegin(GL_QUADS);
		glTexCoord2f(0,1);	glVertex2f(fx - sx, fy - sy);
		glTexCoord2f(1,1);	glVertex2f(fx + sx, fy - sy);
		glTexCoord2f(1,0);	glVertex2f(fx + sx, fy + sy);
		glTexCoord2f(0,0);	glVertex2f(fx - sx, fy + sy);
		glEnd();
	}

			/* RESTORE MODES */

bye:
	gGlobalTransparency = 1.0f;
	OGL_PopState();

}


//===============================================================================================================================================================

#pragma mark -

/*************** INIT CAMERA ***********************/
//
// This MUST be called after the players have been created so that we know
// where to put the camera.
//

void InitCamera(void)
{
ObjNode	*playerObj = gPlayerInfo.objNode;

	ResetCameraSettings();

	if (gLevelNum == LEVEL_NUM_SAUCER)
	{
		InitCamera_Saucer(playerObj);
		return;
	}

			/******************************/
			/* SET CAMERA STARTING COORDS */
			/******************************/


			/* FIRST CHOOSE AN ARBITRARY POINT BEHIND PLAYER */

	if (playerObj)
	{
		float	dx,dz,x,z,r;

		r = playerObj->Rot.y + PI + PI/9; // Use player Rot y directly 
		// r = playerObj->Rot.y;

		dx = sin(r) * 1800.0f;
		dz = cos(r) * 1800.0f;

		dx = 0;
		dz = 0;

		x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x = playerObj->Coord.x + dx;
		z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z = playerObj->Coord.z + dz;
		gGameViewInfoPtr->cameraPlacement.cameraLocation.y = GetTerrainY_Undeformed(x, z) + 500.0f;

		gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = playerObj->Coord.x;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.y = playerObj->Coord.y + gCameraLookAtYOff;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = playerObj->Coord.z;
	}



}

/******************** RESET CAMERA SETTINGS **************************/

static void ResetCameraSettings(void)
{
	gForceCameraAlignment = false;

	gCameraUserRotY = 0;
	mouseCameraAngleY = 0;

	gCameraFromAccel 	= 1; // Always 1 for VR
	gCameraFromAccelY	= 1; // Always 1 for VR


			/* SPECIAL SETTINGS FOR SAUCER LEVEL */
	if (gLevelNum == LEVEL_NUM_SAUCER)
	{
		gCameraLookAtAccel 	= 1; // Always 1 for VR
		gCameraHeightFactor = 0.6;
		gCameraLookAtYOff 	= -200;
		gCameraDistFromMe 	= 800.0f;

	}
	else
	{
		gCameraLookAtAccel 	= 1; // Always 1 for VR

		gCameraHeightFactor = 0.2; // ??
		gCameraLookAtYOff 	= DEFAULT_CAMERA_YOFF; // to be increased to "head height"

		gCameraDistFromMe 	= CAMERA_DEFAULT_DIST_FROM_ME;

	}


}



/*************** UPDATE CAMERA ***************/

void UpdateCamera(void)
{
OGLPoint3D	from,to,target;
float		myX, myY, myZ;
float		fps = gFramesPerSecondFrac;
ObjNode		*playerObj = gPlayerInfo.objNode;
SkeletonObjDataType	*skeleton;
float			oldCamX,oldCamZ,oldCamY,oldPointOfInterestX,oldPointOfInterestZ,oldPointOfInterestY;

// ! NOT USED FOR VR MODE, DEFINED LOWER:
int			firstPersonHeight = VRroomDistanceToGameDistanceScale*2;


// Calculate the HMD's corrected matrix
OGLMatrix4x4 rotOnly = vrInfoHMD.rotationMatrixCorrected;
OGLMatrix4x4 transOnly = vrInfoHMD.translationMatrix;


	// * Debug camera not attached to player
    // printf("Camera at: %.2f, %.2f, %.2f\n",
    //        gGameViewInfoPtr->cameraPlacement.cameraLocation.x,
    //        gGameViewInfoPtr->cameraPlacement.cameraLocation.y,
    //        gGameViewInfoPtr->cameraPlacement.cameraLocation.z);

    // printf("PLAYER gPlayerObj at: %.2f, %.2f, %.2f\n",
    //        playerObj->Coord.x,
    //        playerObj->Coord.y,
    //        playerObj->Coord.z);

	// printf("gPlayerInfor cam location: %.2f, %.2f, %.2f\n",
    //        gPlayerInfo.camera.cameraLocation.x,
    //        gPlayerInfo.camera.cameraLocation.y,
    //        gPlayerInfo.camera.cameraLocation.z);
           
    // printf("Looking at: %.2f, %.2f, %.2f\n",
    //        gGameViewInfoPtr->cameraPlacement.pointOfInterest.x,
    //        gGameViewInfoPtr->cameraPlacement.pointOfInterest.y,
    //        gGameViewInfoPtr->cameraPlacement.pointOfInterest.z);

	if (!playerObj)
		return;

	if (gLevelNum == LEVEL_NUM_SAUCER)
	{
		MoveCamera_Saucer(playerObj);
		return;
	}

	skeleton = playerObj->Skeleton;
	if (!skeleton)
		DoFatalAlert("MoveCamera: player has no skeleton!");

				/******************/
				/* GET COORD DATA */
				/******************/

	oldCamX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;				// get current/old cam coords
	oldCamY = gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
	oldCamZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;

	oldPointOfInterestX = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x;
	oldPointOfInterestY = gGameViewInfoPtr->cameraPlacement.pointOfInterest.y;
	oldPointOfInterestZ = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z;

	myX = playerObj->Coord.x;
	myY = playerObj->Coord.y + playerObj->BottomOff;
	myZ = playerObj->Coord.z;


	if (gPlayerFellIntoBottomlessPit && gPlayerInfo.fellThroughTrapDoor)
	{
		myX = gPlayerInfo.fellThroughTrapDoor->Coord.x;
		myY = gPlayerInfo.fellThroughTrapDoor->Coord.y - 100;
		myZ = gPlayerInfo.fellThroughTrapDoor->Coord.z + TERRAIN_POLYGON_SIZE / 2;
	}

	gPlayerToCameraAngle = PI - CalcYAngleFromPointToPoint(PI-gPlayerToCameraAngle, myX, myZ, oldCamX, oldCamZ);	// calc angle of camera around player



			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/
	
	float forwardDirection = 0;

	float verticalSens = 0.5f; // Reduce vertical axis camera speed

	gCameraControlDelta.y *= verticalSens;
	mouseCameraAngleY += gCameraControlDelta.y; // Add mouse distance travelled (vertical only) to Y dist
	

	// Lock the view to upper max and lower max
	// PI/2 -> Would be directly straight up, and glitches lighting
	// Stay at slightly less than PI to prevent issues
	float limit = PI / 2.1;
	if (mouseCameraAngleY > limit)
		mouseCameraAngleY = limit;
	if (mouseCameraAngleY < -limit)
		mouseCameraAngleY = -limit;
	
	

	// We want the camera to always be able to yaw but it usually rotates the entire player
	// If the game says the player should not move, moving the HMD should move the camera, NOT the player

	bool vrHMDcontrol = true;
	OGLVector3D calculatedUpVector;
	OGLVector3D globalUp = { 0,1,0 };


	if (!vrHMDcontrol) {
		// Basic FPS view, locked to robot facing-forward view:
		to.y = playerObj->Coord.y + firstPersonHeight - sin(mouseCameraAngleY);
		to.x = cos(mouseCameraAngleY) * sin(playerObj->Rot.y + PI) + playerObj->Coord.x;
		to.z = cos(mouseCameraAngleY) * cos(playerObj->Rot.y + PI) + playerObj->Coord.z;
	}
	else {
		// VR HMD Controlled view
		// Set FPS height to VR height
		firstPersonHeight = vrInfoHMD.pos.y + playerEyeHeight; // seems to give reasonable height 
													   // SLIGHTLY too low when standing? but too high when touching floor. To adjust
		// firstPersonHeight to be tested / not sure where to apply this now
		// Set initial to.xyz pos, x & y should be 0, and z -1 * "rotation resolution"
		// Higher negative numbers (-10 or -100 or -1000) seem to provide way smoother rotation than -1
		to.x = 0;
		to.y = 0;
		to.z = -1000;

		OGLPoint3D_Transform(&to, &rotOnly, &to);

		/* DISABLING THIS - USING HMD POSE in OGL_Support.cpp */
		//to.x += playerObj->Coord.x;
		//to.y += playerObj->Coord.y;
		//to.z += playerObj->Coord.z;

		OGLPoint3D_Transform(&to, &transOnly, &to);

		
		// ? To delete? what is this for:
		if (playerObj->StatusBits & STATUS_BIT_NOMOVE) {
			//playerObj->StatusBits &= ~(STATUS_BIT_NOMOVE);
		}
	}
	// ! Not working
		// HMD rotation turns Otto:
		// vrInfoHMD.HMDYawCorrected -= vrInfoHMD.rotDelta.yaw;
	// ! Not working
		// HMD rotation turns Otto:
		// playerObj->Rot.y = vrInfoHMD.HMDYawCorrected;

				/*************************************/
			/* CALC FROM (camera location) POINT */
			/*************************************/


	from.x = playerObj->Coord.x; // ( + a few hundred if needed to see Otto/robot/player for testing )
	from.z = playerObj->Coord.z;	
	from.y = playerObj->Coord.y + firstPersonHeight;


	// Calculate up vector
	calculatedUpVector = globalUp;

	OGL_UpdateCameraFromToUp(&from, &to, &calculatedUpVector);

	/* UPDATE PLAYER'S CAMERA INFO */
	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}


#pragma mark -

/********************** INIT CAMERA:  SAUCER ***************************/

static void InitCamera_Saucer(ObjNode *saucer)
{
float	dx,dz,x,z,r;

	r = saucer->Rot.y + PI + PI/9;

	dx = sin(r) * 1800.0f;
	dz = cos(r) * 1800.0f;

	x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x = saucer->Coord.x + dx;
	z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z = saucer->Coord.z + dz;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y = GetTerrainY_Undeformed(x, z) + 500.0f;

	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = saucer->Coord.x;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.y = saucer->Coord.y + gCameraLookAtYOff;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = saucer->Coord.z;

}

#if 0

/*************************** MOVE CAMERA:  SAUCER ************************************/

static void MoveCamera_Saucer(ObjNode *saucer)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	from,to,*oldPOI,*oldFrom;
OGLPoint3D	targetTo,targetFrom;
OGLVector3D	v;
float		q;
OGLMatrix4x4	m;

		/******************************************/
		/* SEE IF THE USER WILL WANT SOME CONTROL */
		/******************************************/

	if (GetKeyState(kKey_CameraRight))
		gCameraUserRotY += fps * PI;
	else
	if (GetKeyState(kKey_CameraLeft))
		gCameraUserRotY -= fps * PI;

	OGLMatrix4x4_SetRotate_Y(&m, gCameraUserRotY);					// calc the rot matrix


	oldPOI = &gGameViewInfoPtr->cameraPlacement.pointOfInterest;		// get current/old cam coords
	oldFrom = &gGameViewInfoPtr->cameraPlacement.cameraLocation;

			/* CALC FROM */

	targetFrom.x = 0;
	targetFrom.z = 900.0f;
	targetFrom.y = 450.0f;
	OGLPoint3D_Transform(&targetFrom, &m, &targetFrom);				// transform by user rotation
	targetFrom.x += saucer->Coord.x;								// relative to saucer
	targetFrom.z += saucer->Coord.z;
	targetFrom.y += saucer->Coord.y;


	q = fps * 4.0f;
	v.x = (targetFrom.x - oldFrom->x) * q;							// vctor from old to new
	v.y = (targetFrom.y - oldFrom->y) * q;
	v.z = (targetFrom.z - oldFrom->z) * q;

	from.x = oldFrom->x + v.x;										// move from
	from.y = oldFrom->y + v.y;
	from.z = oldFrom->z + v.z;



			/* CALC TO */

	targetTo.x = saucer->Coord.x;
	targetTo.y = saucer->Coord.y - 100.0f;
	targetTo.z = saucer->Coord.z + 200.0f;

	q = fps * 5.0f;
	v.x = (targetTo.x - oldPOI->x) * q;							// vctor from old to new
	v.y = (targetTo.y - oldPOI->y) * q;
	v.z = (targetTo.z - oldPOI->z) * q;

	to.x = oldPOI->x + v.x;										// move to
	to.y = oldPOI->y + v.y;
	to.z = oldPOI->z + v.z;

	OGL_UpdateCameraFromTo(&from,&to);


				/* UPDATE PLAYER'S CAMERA INFO */

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;

	gPlayerToCameraAngle = PI - CalcYAngleFromPointToPoint(PI-gPlayerToCameraAngle, saucer->Coord.x, saucer->Coord.z, from.x, from.z);	// calc angle of camera around saucer
}

#else

/*************************** MOVE CAMERA:  SAUCER ************************************/

static void MoveCamera_Saucer(ObjNode *saucer)
{
OGLPoint3D	from,to,target;
float		distX,distZ,distY,dist;
OGLVector2D	pToC;
float		myX,myY,myZ;
float		fps = gFramesPerSecondFrac;
float			oldCamX,oldCamZ,oldCamY,oldPointOfInterestX,oldPointOfInterestZ,oldPointOfInterestY;

				/******************/
				/* GET COORD DATA */
				/******************/

	oldCamX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;				// get current/old cam coords
	oldCamY = gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
	oldCamZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;

	oldPointOfInterestX = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x;
	oldPointOfInterestY = gGameViewInfoPtr->cameraPlacement.pointOfInterest.y;
	oldPointOfInterestZ = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z;

	myX = saucer->Coord.x;
	myY = saucer->Coord.y + saucer->BottomOff;
	myZ = saucer->Coord.z;

	if (myY < -1400.0f)								// keep from looking straight down if fallen into bottomless pit
		myY = -1400.0f;

	gPlayerToCameraAngle = PI - CalcYAngleFromPointToPoint(PI-gPlayerToCameraAngle, myX, myZ, oldCamX, oldCamZ);	// calc angle of camera around player

		/******************************************/
		/* SEE IF THE USER WILL WANT SOME CONTROL */
		/******************************************/

	if (gAutoRotateCamera)
	{
		gCameraUserRotY += fps * gAutoRotateCameraSpeed;
	}
	else if (gGamePrefs.snappyCameraControl && gCameraControlDelta.x != 0)
	{
		gCameraUserRotY += fps * PI * gCameraControlDelta.x;
		gForceCameraAlignment = true;								// don't zero userRot out if the player isn't moving
	}

	if (GetNeedState(kNeed_CameraMode))											// see if re-align camera behind player
	{
		gCameraUserRotY = 0;
		gForceCameraAlignment = true;
	}
	else if (gGamePrefs.snappyCameraControl
		&& gCameraControlDelta.x != 0
		&& !gAutoRotateCamera)
	{
		// force camera alignment is ALWAYS overridden by user
		gForceCameraAlignment = false;
	}


			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/

	target.x = myX;								// accelerate "to" toward target "to"
	target.y = myY + gCameraLookAtYOff;
	target.z = myZ;

	distX = target.x - oldPointOfInterestX;
	distY = target.y - oldPointOfInterestY;
	distZ = target.z - oldPointOfInterestZ;

	if (distX > 350.0f)
		distX = 350.0f;
	else
	if (distX < -350.0f)
		distX = -350.0f;

	if (distZ > 350.0f)
		distZ = 350.0f;
	else
	if (distZ < -350.0f)
		distZ = -350.0f;


	to.x = oldPointOfInterestX + (distX * (fps * gCameraLookAtAccel));
	to.y = oldPointOfInterestY + (distY * (fps * gCameraLookAtAccel));
	to.z = oldPointOfInterestZ + (distZ * (fps * gCameraLookAtAccel));


			/*******************/
			/* CALC FROM POINT */
			/*******************/

	if (gFreezeCameraFromXZ)
	{
		from.x = target.x = oldCamX;
		from.z = target.x = oldCamZ;
	}
	else
	{
				/* CALC PLAYER->CAMERA VECTOR */

		pToC.x = oldCamX - myX;												// calc player->camera vector
		pToC.y = oldCamZ - myZ;
		OGLVector2D_Normalize(&pToC,&pToC);									// normalize it


				/* MOVE TO BEHIND PLAYER */

		// * Should never be done in VR
				// if (gForceCameraAlignment
		// 	|| (gGamePrefs.autoAlignCamera && gTimeSinceLastThrust > 1.1f))			// don't auto-align if player is being moved & we are not forcing it
		// {
		// 	float	r,ratio;
		// 	OGLVector2D	behind;

		// 			/* CALC PLAYER BEHIND VECTOR */

		// 	r = CalcYAngleFromPointToPoint(0, myX, myZ, myX + saucer->Delta.x, myZ + saucer->Delta.z);		// calc angle based on deltas since saucer doesn't have an aim per-se
		// 	r += gCameraUserRotY;										// offset by user rot

		// 	behind.x = sin(r);
		// 	behind.y = cos(r);

		// 			/* WEIGH THE VECTORS */

		// 	ratio = .5f;
		// 	pToC.x = (pToC.x * (1.0f - ratio)) + (behind.x * ratio);
		// 	pToC.y = (pToC.y * (1.0f - ratio)) + (behind.y * ratio);

		// 	OGLVector2D_Normalize(&pToC,&pToC);

		// 	if ((fabs(pToC.x) <= EPS) && (fabs(pToC.y) <= EPS))			// check if looking straight @ player's front
		// 	{
		// 		pToC.x = 1;										// assign some random vector
		// 		pToC.y = 0;
		// 	}
		// }

		dist = gCameraDistFromMe;


		target.x = myX + (pToC.x * dist);									// target is appropriate dist based on cam's current coord
		target.z = myZ + (pToC.y * dist);


				/* MOVE CAMERA TOWARDS POINT */

		distX = target.x - oldCamX;
		distZ = target.z - oldCamZ;

		distX = distX * (fps * gCameraFromAccel);
		distZ = distZ * (fps * gCameraFromAccel);

		from.x = oldCamX+distX;
		from.z = oldCamZ+distZ;
	}



		/***************/
		/* CALC FROM Y */
		/***************/

	if (gFreezeCameraFromY)
	{
		from.y = oldCamY;
	}
	else
	{
		float	camBottom,fenceTopY,waterY;
		Boolean	overTop;
		OGLPoint3D	intersect;

		dist = CalcQuickDistance(from.x, from.z, to.x, to.z) - CAMERA_CLOSEST;
		if (dist < 0.0f)
			dist = 0.0f;

		target.y = myY + (dist*gCameraHeightFactor);						// calc desired y based on dist and height factor


			/* CHECK CLEAR LINE OF SIGHT WITH FENCE */

		if (SeeIfLineSegmentHitsFence(&target, &to, &intersect,&overTop,&fenceTopY))			// see if intersection with fence
		{
			if (!overTop)
			{
				target.y = fenceTopY += 200.0f;
			}
		}


			/*******************************/
			/* MOVE ABOVE ANY SOLID OBJECT */
			/*******************************/
			//
			// well... not any solid object, just reasonable ones
			//

		camBottom = target.y - 100.0f;
		if (DoSimpleBoxCollision(target.y + 100.0f, camBottom,
								from.x - 150.0f, from.x + 150.0f,
								from.z + 150.0f, from.z - 150.0f,
								CTYPE_BLOCKCAMERA))
		{
			ObjNode *obj = gCollisionList[0].objectPtr;					// get collided object
			if (obj)
			{
				target.y = obj->CollisionBoxes[0].top + 100.0f;				// set target on top of object
			}
		}

		dist = (target.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y)*gCameraFromAccelY;	// calc dist from current y to desired y
		from.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y+(dist*fps);

		if (gPlayerInfo.scaleRatio <= 1.0f)
			if (from.y < (to.y+CAM_MINY))								// make sure camera never under the "to" point (insures against camera flipping over)
				from.y = (to.y+CAM_MINY);


				/* MAKE SURE NOT UNDERGROUND OR WATER */

		dist = GetTerrainY2(from.x, from.z) + gMinHeightOffGround;
		if (from.y < dist)
			from.y = dist;

		if (GetWaterY(from.x, from.z, &waterY))
		{
			waterY += 50.0f;
			if (from.y < waterY)
				from.y = waterY;
		}
	}


				/**********************/
				/* UPDATE CAMERA INFO */
				/**********************/

	if (gGamePrefs.snappyCameraControl
		&& gCameraControlDelta.x != 0
		&& !gAutoRotateCamera)
	{
		gTimeSinceLastThrust = -1000;
		gForceCameraAlignment = false;

		OGLMatrix4x4	m;
		float			r = gCameraControlDelta.x * fps * 2.5f;
		OGLMatrix4x4_SetRotateAboutPoint(&m, &to, 0, r, 0);
		OGLPoint3D_Transform(&from, &m, &from);
	}


	if (gAutoRotateCamera && gFreezeCameraFromXZ)				// special auto-rot for frozen xz condition
	{
		OGLMatrix4x4	m;
		OGLMatrix4x4_SetRotateAboutPoint(&m, &to, 0, fps * gAutoRotateCameraSpeed, 0);
		OGLPoint3D_Transform(&from, &m, &from);
	}


	OGL_UpdateCameraFromTo(&from,&to);


				/* UPDATE PLAYER'S CAMERA INFO */

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}


#endif



#pragma mark -

/********************** PREP ANAGLYPH CAMERAS ***************************/
//
// Make a copy of the camera's real coord info before we do anaglyph offsetting.
//

void PrepAnaglyphCameras(void)
{
		gAnaglyphCameraBackup = gGameViewInfoPtr->cameraPlacement;


}

/********************** RESTORE CAMERAS FROM ANAGLYPH ***************************/

void RestoreCamerasFromAnaglyph(void)
{
	gGameViewInfoPtr->cameraPlacement = gAnaglyphCameraBackup;
}


/******************** CALC ANAGLYPH CAMERA OFFSET ***********************/

void CalcAnaglyphCameraOffset(Byte pass)
{
OGLVector3D	aim;
OGLVector3D	xaxis;
float		sep = gAnaglyphEyeSeparation;

	if (pass > 0)
		sep = -sep;

			/* CALC CAMERA'S X-AXIS */

	aim.x = gAnaglyphCameraBackup.pointOfInterest.x - gAnaglyphCameraBackup.cameraLocation.x;
	aim.y = gAnaglyphCameraBackup.pointOfInterest.y - gAnaglyphCameraBackup.cameraLocation.y;
	aim.z = gAnaglyphCameraBackup.pointOfInterest.z - gAnaglyphCameraBackup.cameraLocation.z;
	OGLVector3D_Normalize(&aim, &aim);

	OGLVector3D_Cross(&gAnaglyphCameraBackup.upVector, &aim, &xaxis);


				/* OFFSET CAMERA FROM */

	gGameViewInfoPtr->cameraPlacement.cameraLocation.x = gAnaglyphCameraBackup.cameraLocation.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z = gAnaglyphCameraBackup.cameraLocation.z + (xaxis.z * sep);
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y = gAnaglyphCameraBackup.cameraLocation.y + (xaxis.y * sep);



				/* OFFSET CAMERA TO */

	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = gAnaglyphCameraBackup.pointOfInterest.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = gAnaglyphCameraBackup.pointOfInterest.z + (xaxis.z * sep);
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.y = gAnaglyphCameraBackup.pointOfInterest.y + (xaxis.y * sep);



}








