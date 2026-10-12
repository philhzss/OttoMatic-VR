#include "openvr.h"
#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "game.h"
#include <iostream>
#include <cstring>
#include <SDL3/SDL_filesystem.h>


struct VRActionHandlers // vrActions is an instance of this
{
	vr::VRActionHandle_t MoveXY;
	vr::VRActionHandle_t CameraXY;
	vr::VRActionHandle_t Shoot;
	vr::VRActionHandle_t Jump;
	vr::VRActionHandle_t FistLeft;
	vr::VRActionHandle_t FistRight;
	vr::VRActionHandle_t PunchOrPickUp;
	vr::VRActionHandle_t PreviousWeapon;
	vr::VRActionHandle_t NextWeapon;
	vr::VRActionHandle_t EscapePause;
	vr::VRActionHandle_t CheatButton; 

	vr::VRActionHandle_t Vibration;


};

static vr::VRInputValueHandle_t notRestrictedToHand = vr::k_ulInvalidInputValueHandle;
static vr::VRInputValueHandle_t rightHandOnly;
static vr::VRInputValueHandle_t leftHandOnly;

static VRActionHandlers vrActions{}; // init
static vr::VRActionSetHandle_t ottoNormalGameplayActions;
static vr::VRActionSetHandle_t ottoLevel9GameplayActions;
static vr::VRActionSetHandle_t ottoMenuActions;
static vr::VRActionSetHandle_t ottoHaptics;

static vr::VRActiveActionSet_t activeActionSets[2]{};
static vr::VRActionSetHandle_t actionSetTable[3]; // index = enum value


static vr::InputAnalogActionData_t moveXYdata{};
static vr::InputAnalogActionData_t cameraXYdata{};
static vr::InputAnalogActionData_t shootData{};
static vr::InputAnalogActionData_t fistLeftData{};
static vr::InputAnalogActionData_t fistRightData{};
static vr::InputDigitalActionData_t jumpData{};
static vr::InputDigitalActionData_t punchOrPickupData{};
static vr::InputDigitalActionData_t nextWeaponData{};
static vr::InputDigitalActionData_t previousWeaponData{};
static vr::InputDigitalActionData_t EscapePauseData{};
static vr::InputDigitalActionData_t cheatButtonData{};

static vr::InputDigitalActionData_t MenuEnterData{};
static vr::InputDigitalActionData_t MenuUpData{};
static vr::InputDigitalActionData_t MenuDownData{};
static vr::InputDigitalActionData_t MenuLeftData{};
static vr::InputDigitalActionData_t MenuRightData{};

// Action set switcher (haptic action set always active) 
extern "C" void vrcpp_SetMainActionSet(enum VRActionSetID which) {
	activeActionSets[0].ulActionSet = actionSetTable[which];
}

extern "C" void vrcpp_initSteamVRInput(void) {
	// Add path to action manifest for VR controls
	std::string actionPathCPP = std::string(SDL_GetBasePath()) + "Data\\steamvr\\actions.json";
	const char *actionPath = actionPathCPP.c_str();
	vr::VRInput()->SetActionManifestPath(actionPath);


	// Get handles for VR action sets and actions
	auto error = vr::VRInput()->GetActionSetHandle("/actions/ottoGameplay", &ottoNormalGameplayActions);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionSetHandle error.\n";
		printf("GetActionSetHandle error.\n");
	}

	error = vr::VRInput()->GetActionSetHandle("/actions/menus", &ottoMenuActions);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionSetHandle error.\n";
		printf("GetActionSetHandle error.\n");

	}
	error = vr::VRInput()->GetActionSetHandle("/actions/haptics", &ottoHaptics);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionSetHandle error.\n";
		printf("GetActionSetHandle error.\n");
	}


	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/MoveXY", &vrActions.MoveXY);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/CameraXY", &vrActions.CameraXY);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/Shoot", &vrActions.Shoot);
	error = vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/Jump", &vrActions.Jump);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionHandle error.\n";
		printf("GetActionHandle error.\n");
	}
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/FistLeft", &vrActions.FistLeft);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/FistRight", &vrActions.FistRight);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/PunchOrPickUp", &vrActions.PunchOrPickUp);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/PreviousWeapon", &vrActions.PreviousWeapon);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/NextWeapon", &vrActions.NextWeapon);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/EscapePause", &vrActions.EscapePause);
	vr::VRInput()->GetActionHandle("/actions/ottoGameplay/in/CheatButton", &vrActions.CheatButton);  
	vr::VRInput()->GetActionHandle("/actions/haptics/out/Haptic", &vrActions.Vibration);

	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &leftHandOnly);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &rightHandOnly);

	actionSetTable[VR_ACTIONSET_GAMEPLAY] = ottoNormalGameplayActions;
	actionSetTable[VR_ACTIONSET_LEVEL9]   = ottoNormalGameplayActions; // placeholder until the level 9 set exists (ottoLevel9GameplayActions)
	actionSetTable[VR_ACTIONSET_MENU]     = ottoMenuActions;

	
	vrcpp_SetMainActionSet(VR_ACTIONSET_GAMEPLAY);
	activeActionSets[0].ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;

	activeActionSets[1].ulActionSet = ottoHaptics;
	activeActionSets[1].ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;	
}

extern "C" void vrcpp_UpdateActionState(void) {
	auto error = vr::VRInput()->UpdateActionState(&activeActionSets[0], sizeof(activeActionSets[0]), 2);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "Error UpdateActionState.\n";
	}
}



extern "C" vrJoyPos vrcpp_GetAnalogActionData(int actionToDo) {
	vr::VRActionHandle_t actionHandler;
	vr::InputAnalogActionData_t actionDataStruct;
	switch (actionToDo) {
	case playerActions::vrCameraXY:
		actionHandler = vrActions.CameraXY;
		actionDataStruct = cameraXYdata;
		break;
	case playerActions::vrMoveXY:
		actionHandler = vrActions.MoveXY;
		actionDataStruct = moveXYdata;
		break;
	case playerActions::vrShoot:
		actionHandler = vrActions.Shoot;
		actionDataStruct = shootData;
		break;
	case playerActions::vrFistLeft:
		actionHandler = vrActions.FistLeft;
		actionDataStruct = fistLeftData;
		break;
	case playerActions::vrFistRight:
		actionHandler = vrActions.FistRight;
		actionDataStruct = fistRightData;
		break;
	default:
		printf("vrcpp_GetAnalogActionData called incorrectly");
		return { 0,0 };
	}

	// Get the state of the action
	vr::VRInput()->GetAnalogActionData(actionHandler, &actionDataStruct, sizeof(actionDataStruct), notRestrictedToHand);
	if (!actionDataStruct.bActive) {
		// printf("GetAnalogActionData Problem, available to be bound: no\n"); // If this is printed, something wrong
	}

	// If any movement on any axis, return something other than 0
	if (actionDataStruct.x || actionDataStruct.y || actionDataStruct.z) {
		// For now all my actions are vector2 so only the XY is useful.
		// Must change this if we use vector3 actions eventually!!!
		return { actionDataStruct.x, actionDataStruct.y };
	}
	else {
		return { 0,0 };
	}
}



extern "C" bool vrcpp_GetDigitalActionData(int actionToDo, bool preventPressAndHold) {
	vr::VRActionHandle_t actionHandler;
	vr::InputDigitalActionData_t actionDataStruct;
	switch (actionToDo) {
	case playerActions::vrJump:
		actionHandler = vrActions.Jump;
		actionDataStruct = jumpData;
		break;
	case playerActions::vrPunchOrPickUp:
		actionHandler = vrActions.PunchOrPickUp;
		actionDataStruct = punchOrPickupData;
		break;
	case playerActions::vrPreviousWeapon:
		actionHandler = vrActions.PreviousWeapon;
		actionDataStruct = previousWeaponData;
		break;
	case playerActions::vrNextWeapon:
		actionHandler = vrActions.NextWeapon;
		actionDataStruct = nextWeaponData;
		break;
	case playerActions::vrEscapePause:
		actionHandler = vrActions.EscapePause;
		actionDataStruct = EscapePauseData;
		break;
	case playerActions::vrCheatButton:
		actionHandler = vrActions.CheatButton;
		actionDataStruct = cheatButtonData;
		break;
	default:
		printf("vrcpp_GetDigitalActionData called incorrectly");
		return false;
	}

	// Get the state of the action
	vr::VRInput()->GetDigitalActionData(actionHandler, &actionDataStruct, sizeof(actionDataStruct), notRestrictedToHand);
	if (!actionDataStruct.bActive) {
		// printf("vrcpp_GetDigitalActionData Problem, available to be bound: no\n"); // If this is printed, something wrong
	}
	if (actionDataStruct.bState && actionDataStruct.bChanged) {
		// printf("Changed\n"); // Use to test, prints if command detects
		if (preventPressAndHold) {
			if (actionDataStruct.fUpdateTime >= -0.02) {
				printf("Update Time %f is >= -0.02, command allowed\n", actionDataStruct.fUpdateTime);
				return true;
			}
			else {
				printf("Update Time %f is < -0.02, BLOCKED, potential press & hold\n", actionDataStruct.fUpdateTime);
				return false;
			}
		}
		else {
			// If not preventing pressAndHold, return true here since button state pressed
			return true;
		}
	}
	else {
		return false;
	}
}

extern "C" void vrcpp_DoVibrationHaptics(int handToVibrate,
	float fDurationSeconds, float fFrequency, float fAmplitude)
{
	vr::VRInputValueHandle_t deviceRestriction;
	switch (handToVibrate) {
	case playerActions::vrLeftVibrate:
		deviceRestriction = leftHandOnly;
		break;
	case playerActions::vrRightVibrate:
		deviceRestriction = rightHandOnly;
		break;
	case playerActions::vrBothVibrate:
		deviceRestriction = notRestrictedToHand;
		break;
	default:
		printf("vrcpp_GetDigitalActionData called incorrectly");
		return;
	}

	vr::VRInput()->TriggerHapticVibrationAction(vrActions.Vibration, 0, fDurationSeconds, fFrequency, fAmplitude, deviceRestriction);

}

// Speed default 3
extern "C" void vrcpp_DoVRthumbstickCamera(float rotationSpeed) {
		float  VRcameraJoyPostionX = vrcpp_GetAnalogActionData(vrCameraXY).x;
		// ? VRcameraJoyPostionX /= 30; // Reduce for sensitivty

		// MULTIPLY by frame time for smooth rotation!
		float deltaTime = gFramesPerSecondFrac;  // Frame time in seconds

		vrInfoHMD.camThumbstickAccum -= VRcameraJoyPostionX * rotationSpeed * deltaTime;


		vrInfoHMD.HMDYawCorrected = vrInfoHMD.camThumbstickAccum;
		// printf("\nROTATE X: %f                  ", VRcameraJoyPostionX);
	}


