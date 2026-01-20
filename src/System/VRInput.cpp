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
	vr::VRActionHandle_t EscapeMenu;
	vr::VRActionHandle_t CheatButton; 
	vr::VRActionHandle_t Vibration;
};

static vr::VRInputValueHandle_t notRestrictedToHand = vr::k_ulInvalidInputValueHandle;
static vr::VRInputValueHandle_t rightHandOnly;
static vr::VRInputValueHandle_t leftHandOnly;

static VRActionHandlers vrActions{}; // init
static vr::VRActionSetHandle_t ottoVRactions;

static vr::VRActiveActionSet_t activeActionSet;


static vr::InputAnalogActionData_t moveXYAction{};
static vr::InputAnalogActionData_t cameraXYAction{};
static vr::InputAnalogActionData_t shootAction{};
static vr::InputAnalogActionData_t fistLeftAction{};
static vr::InputAnalogActionData_t fistRightAction{};
static vr::InputDigitalActionData_t jumpAction{};
static vr::InputDigitalActionData_t punchOrPickupAction{};
static vr::InputDigitalActionData_t nextWeaponAction{};
static vr::InputDigitalActionData_t previousWeaponAction{};
static vr::InputDigitalActionData_t EscapeMenuAction{};
static vr::InputDigitalActionData_t cheatButtonAction{};


extern "C" void vrcpp_initSteamVRInput(void) {
	// Add path to action manifest for VR controls
	std::string actionPathCPP = std::string(SDL_GetBasePath()) + "Data\\steamvr\\actions.json";
	const char *actionPath = actionPathCPP.c_str();
	vr::VRInput()->SetActionManifestPath(actionPath);


	// Get handles for VR action sets and actions
	auto error = vr::VRInput()->GetActionSetHandle("/actions/otto", &ottoVRactions);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionSetHandle error.\n";
		printf("GetActionSetHandle error.\n");
	}


	vr::VRInput()->GetActionHandle("/actions/otto/in/MoveXY", &vrActions.MoveXY);
	vr::VRInput()->GetActionHandle("/actions/otto/in/CameraXY", &vrActions.CameraXY);
	vr::VRInput()->GetActionHandle("/actions/otto/in/Shoot", &vrActions.Shoot);
	error = vr::VRInput()->GetActionHandle("/actions/otto/in/Jump", &vrActions.Jump);
	if (error != vr::EVRInputError::VRInputError_None)
	{
		std::cerr << "GetActionHandle error.\n";
		printf("GetActionHandle error.\n");
	}
	vr::VRInput()->GetActionHandle("/actions/otto/in/FistLeft", &vrActions.FistLeft);
	vr::VRInput()->GetActionHandle("/actions/otto/in/FistRight", &vrActions.FistRight);
	vr::VRInput()->GetActionHandle("/actions/otto/in/PunchOrPickup", &vrActions.PunchOrPickUp);
	vr::VRInput()->GetActionHandle("/actions/otto/in/PreviousWeapon", &vrActions.PreviousWeapon);
	vr::VRInput()->GetActionHandle("/actions/otto/in/NextWeapon", &vrActions.NextWeapon);
	vr::VRInput()->GetActionHandle("/actions/otto/in/EscapeMenu", &vrActions.EscapeMenu);
	vr::VRInput()->GetActionHandle("/actions/otto/in/CheatButton", &vrActions.CheatButton);  
	vr::VRInput()->GetActionHandle("/actions/otto/out/Haptic", &vrActions.Vibration);

	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &leftHandOnly);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &rightHandOnly);


	activeActionSet.ulActionSet = ottoVRactions;
	activeActionSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
	activeActionSet.nPriority = 0; // might be needed? Unsure
}

extern "C" void vrcpp_UpdateActionState(void) {
	auto error = vr::VRInput()->UpdateActionState(&activeActionSet, sizeof(activeActionSet), 1);
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
		actionDataStruct = cameraXYAction;
		break;
	case playerActions::vrMoveXY:
		actionHandler = vrActions.MoveXY;
		actionDataStruct = moveXYAction;
		break;
	case playerActions::vrShoot:
		actionHandler = vrActions.Shoot;
		actionDataStruct = shootAction;
		break;
	case playerActions::vrFistLeft:
		actionHandler = vrActions.FistLeft;
		actionDataStruct = fistLeftAction;
		break;
	case playerActions::vrFistRight:
		actionHandler = vrActions.FistRight;
		actionDataStruct = fistRightAction;
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
		actionDataStruct = jumpAction;
		break;
	case playerActions::vrPunchOrPickUp:
		actionHandler = vrActions.PunchOrPickUp;
		actionDataStruct = punchOrPickupAction;
		break;
	case playerActions::vrPreviousWeapon:
		actionHandler = vrActions.PreviousWeapon;
		actionDataStruct = previousWeaponAction;
		break;
	case playerActions::vrNextWeapon:
		actionHandler = vrActions.NextWeapon;
		actionDataStruct = nextWeaponAction;
		break;
	case playerActions::vrEscapeMenu:
		actionHandler = vrActions.EscapeMenu;
		actionDataStruct = EscapeMenuAction;
		break;
	case playerActions::vrCheatButton:
		actionHandler = vrActions.CheatButton;
		actionDataStruct = cheatButtonAction;
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
