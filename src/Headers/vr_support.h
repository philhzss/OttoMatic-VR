#pragma once

		// * INPUT * //

// This is used for calling Get*ActionData, to tell SteamVR Input which control triggered
enum playerActions
{
	vrMoveXY = 0,
	vrCameraXY,
	vrJump,
	vrShoot,
	vrFistLeft,
	vrFistRight,
	vrPunchOrPickUp,
	vrPreviousWeapon,
	vrNextWeapon,
	vrEscapeMenu,
	vrCheatButton,
	vrLeftVibrate,
	vrRightVibrate,
	vrBothVibrate
};

#define VRminimumTriggerDefault 0.7f // How far the trigger has to be pulled for most trigger actions
#define VRminimumThumbstickDefault 0.4f // How far you must move thumbstick for most menu actions


extern float VRroomDistanceToGameDistanceScale; // Defined in  OGL support
extern float VRroomDistanceToGameDistanceScaleDEFAULT; // Defined in  OGL support, what to return to
extern int playerEyeHeight; // Define in OGL support



// Virtual IPD
extern float gIpdScale; // Defined in OGL support
extern float gIpdScaleDEFAULT; // Defined in OGL support, what to return to
extern float gWorldScale; // Defined in OGL support


typedef enum
{
	VR_DEVICE_UNKNOWN = 0,
	VR_DEVICE_HMD,
	VR_DEVICE_CONTROLLER,
	VR_DEVICE_TRACKER,
	VR_DEVICE_BASESTATION
} vrDeviceType;

typedef struct
{
	float x;
	float y;
} vrJoyPos;

typedef struct
{
	double pitch;
	double yaw;
	double roll;
} vrEuler;

typedef struct
{
	float x;
	float y;
	float z;
} vrPosition;

typedef struct 
{
	float m[3][4];
}vrMatrix34;


typedef struct
{
	double w, x, y, z;
}vrQuaternion;


		// * TRACKING * //

typedef struct
{
	int deviceID;
	vrDeviceType deviceType;
	vrQuaternion quat;


		/* ROTATION EULER (pitch, yaw, roll) (Probably should not use this, gimbal lock) */
	vrEuler rot; // Current actual rotation
	vrEuler rotDelta; // Rotation delta (dif since last frame/last check)
	OGLMatrix4x4 scaledPlayspaceTransformMatrix; // NOT corrected for gameYaw
	OGLMatrix4x4 worldSpaceTransformMatrix; // CORRECTED for gameYaw
	OGLMatrix4x4 worldSpaceRotationMatrix; // CORRECTED for gameYaw
	OGLMatrix4x4 scaledPlayspaceTranslationMatrix; // NOT corrected for gameYaw


		// Rotation special
	double camThumbstickAccum; // Track camera rotation for just the thumbstick, no HMD
	double HMDYawCorrected; // Only useful for HMD, use to correct yaw from thumbstick rotation
	double HMDgameYawIgnoringHMD; // Corrects for the gameYaw (worldspace), the X & Z directions change with thumbstick
	OGLMatrix4x4 gameYawCorrectionMatrix; // Apply this to tracked devices BEFORE anything else


		/* POSITION (x, y, z) */
	vrPosition pos; // Current actual position
	vrPosition posDelta; // Position delta (dif since last frame/last check)

	
		/* HMD Projection View */
	OGLMatrix4x4 HMDleftProj;
	OGLMatrix4x4 HMDrightProj;
	OGLMatrix4x4 HMDeyeToHeadLeft;
	OGLMatrix4x4 HMDeyeToHeadRight;
	uint32_t gEyeTargetWidth;
	uint32_t gEyeTargetHeight;


}TrackedVrDeviceInfo;

#ifdef __cplusplus
extern "C" {
#endif
	extern TrackedVrDeviceInfo vrInfoHMD;
	extern TrackedVrDeviceInfo vrInfoLeftHand;
	extern TrackedVrDeviceInfo vrInfoRightHand;
	extern OGLPoint3D gVRPlayspaceCenter;
	extern float RotateOffsetByTotalYaw(float offsetX, float offsetZ, float* outX, float* outZ);
#ifdef __cplusplus
}
#endif