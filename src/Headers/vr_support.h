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
	vrLeftVibrate,
	vrRightVibrate,
	vrBothVibrate
};

#define VRminimumTriggerDefault 0.7f // How far the trigger has to be pulled for most trigger actions
#define VRminimumThumbstickDefault 0.4f // How far you must move thumbstick for most menu actions

// Multiply tracked VR device pos by this to get the equivalent game distance
// This number is APPROXIMATE and should be tweaked as playtesting happens
#define VRroomDistanceToGameDistanceScale 250

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
	vrMatrix34 rawVRmatrix;
	vrQuaternion quat;


		/* ROTATION EULER (pitch, yaw, roll) (Probably should not use this, gimbal lock) */
	vrEuler rot; // Current actual rotation
	vrEuler rotDelta; // Rotation delta (dif since last frame/last check)
	OGLMatrix4x4 transformationMatrix; // NOT corrected for gameYaw
	OGLMatrix4x4 transformationMatrixInverted;
	OGLMatrix4x4 transformationMatrixCorrected; // CORRECTED for gameYaw
	OGLMatrix4x4 rotationMatrixCorrected; // CORRECTED for gameYaw
	OGLMatrix4x4 translationMatrix; // NOT corrected for gameYaw


		// Rotation special
	double camThumbstickAccum; // Track camera rotation for just the thumbstick, no HMD
	double HMDYawCorrected; // Only useful for HMD, use to correct yaw from thumbstick rotation
	double HMDgameYawIgnoringHMD; // Corrects for the gameYaw (worldspace), the X & Z directions change with thumbstick
	OGLMatrix4x4 HMDgameYawCorrectionMatrix; // Apply this to tracked devices BEFORE anything else


		/* POSITION (x, y, z) */
	vrPosition pos; // Current actual position
	vrPosition posDelta; // Position delta (dif since last frame/last check)
	vrPosition posGameAxes; // Position based in the game worldspace
	vrPosition posDeltaGameAxes; // Position based in the game worldspace delta

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
#ifdef __cplusplus
}
#endif