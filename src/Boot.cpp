// OTTO MATIC ENTRY POINT
// (C) 2025 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "openvr.h"
vr::IVRSystem *gIVRSystem;
float gIpdScale = 100.0f; // A larger number makes the world appear smaller

extern "C"
{
	#include "game.h"

	SDL_Window* gSDLWindow = nullptr;
	FSSpec gDataSpec;
	int gCurrentAntialiasingLevel;
}

static fs::path FindGameData(const char* executablePath)
{
	fs::path dataPath;

	int attemptNum = 0;

#if !(__APPLE__)
	attemptNum++;		// skip macOS special case #0
#endif

	if (!executablePath)
		attemptNum = 2;

tryAgain:
	switch (attemptNum)
	{
		case 0:			// special case for macOS app bundles
			dataPath = executablePath;
			dataPath = dataPath.parent_path().parent_path() / "Resources";
			break;

		case 1:
			dataPath = executablePath;
			dataPath = dataPath.parent_path() / "Data";
			break;

		case 2:
			dataPath = "Data";
			break;

		default:
			throw std::runtime_error("Couldn't find the Data folder.");
	}

	attemptNum++;

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System");

	FSSpec someDataFileSpec;
	OSErr iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":System:gamecontrollerdb.txt", &someDataFileSpec);
	if (iErr)
	{
		goto tryAgain;
	}

	return dataPath;
}

void GetIVRErrorString(char errorString[255], vr::HmdError peError)
{
	switch(peError)
	{
	case vr::VRInitError_None:
		strcpy(errorString, "HmdError_None(0) - There was no error");
		break;

	case vr::VRInitError_Unknown:
		strcpy(errorString, "HmdError_Unknown(1) - There was an unknown error");
		break;

	case vr::VRInitError_Init_InstallationNotFound:
		strcpy(errorString, "HmdError_Init_InstallationNotFound(100) - The installation folder specified in the path registry doesn't exist.");
		break;

	case vr::VRInitError_Init_InstallationCorrupt:
		strcpy(errorString, "HmdError_Init_InstallationCorrupt(101) - The installation folder specified in the path registry has no bin folder.");
		break;

	case vr::VRInitError_Init_VRClientDLLNotFound:
		strcpy(errorString, "HmdError_Init_VRClientDLLNotFound(102) - The bin folder has no vrclient.dll (or system - appropriate dynamic library).");
		break;

	case vr::VRInitError_Init_FileNotFound:
		strcpy(errorString, "HmdError_Init_FileNotFound(103) - A driver could not be loaded.");
		break;

	case vr::VRInitError_Init_FactoryNotFound:
		strcpy(errorString, "HmdError_Init_FactoryNotFound(104) - The factory function in vrclient.dll could not be found.Is vrclient.dll corrupt ?");
		break;

	case vr::VRInitError_Init_InterfaceNotFound:
		strcpy(errorString, "HmdError_Init_InterfaceNotFound(105) - The specific interface function requested by VR_Init or VR_GetGenericInterface could not be found. Is the SDK being used newer than the installed runtime ?");
		break;

	case vr::VRInitError_Init_InvalidInterface:
		strcpy(errorString, "HmdError_Init_InvalidInterface(106) - This error code is currently unused.");
		break;

	case vr::VRInitError_Init_UserConfigDirectoryInvalid:
		strcpy(errorString, "HmdError_Init_UserConfigDirectoryInvalid(107) - The config directory specified in the path registry was not writable.");
		break;

	case vr::VRInitError_Init_HmdNotFound:
		strcpy(errorString, "HmdError_Init_HmdNotFound(108) - Either no HMD was attached to the system or the HMD could not be initialized.");
		break;

	case vr::VRInitError_Init_NotInitialized:
		strcpy(errorString, "HmdError_Init_NotInitialized(109) - VR_GetGenericInterface will return this error if it is called before VR_Init or after VR_Shutdown.");
		break;

	case vr::VRInitError_Init_PathRegistryNotFound:
		strcpy(errorString, "HmdError_Init_PathRegistryNotFound(110) - The VR path registry file could not be read. Reinstall the OpenVR runtime (or the SteamVR application on Steam.)");
		break;

	case vr::VRInitError_Init_NoConfigPath:
		strcpy(errorString, "HmdError_Init_NoConfigPath(111) - The config path was not specified in the path registry.");
		break;

	case vr::VRInitError_Init_NoLogPath:
		strcpy(errorString, "HmdError_Init_NoLogPath(112) - The log path was not specified in the path registry.");
		break;

	case vr::VRInitError_Init_PathRegistryNotWritable:
		strcpy(errorString, "HmdError_Init_PathRegistryNotWritable(113) - The VR path registry could not be written.");
		break;

	case vr::VRInitError_Driver_Failed:
		strcpy(errorString, "HmdError_Driver_Failed(200) - A driver failed to initialize. This is an internal error.");
		break;

	case vr::VRInitError_Driver_Unknown:
		strcpy(errorString, "HmdError_Driver_Unknown(201) - A driver failed for an unknown reason. This is an internal error.");
		break;

	case vr::VRInitError_Driver_HmdUnknown:
		strcpy(errorString, "HmdError_Driver_HmdUnknown(202) - A driver did not detect an HMD. This is an internal error.");
		break;

	case vr::VRInitError_Driver_NotLoaded:
		strcpy(errorString, "HmdError_Driver_NotLoaded(203) - A driver was not loaded before requests were made from that driver. This is an internal error.");
		break;

	case vr::VRInitError_Driver_RuntimeOutOfDate:
		strcpy(errorString, "HmdError_Driver_RuntimeOutOfDate(204) - For drivers with a runtime of their own, that runtime needs to be updated.");
		break;

	case vr::VRInitError_Driver_HmdInUse:
		strcpy(errorString, "HmdError_Driver_HmdInUse(205) - Another non - OpenVR application is using the HMD.");
		break;

	case vr::VRInitError_IPC_ServerInitFailed:
		strcpy(errorString, "HmdError_IPC_ServerInitFailed(300) - OpenVR was unable to start vrserver.");
		break;

	case vr::VRInitError_IPC_ConnectFailed:
		strcpy(errorString, "HmdError_IPC_ConnectFailed(301) - After repeated attempts, OpenVR was unable to connect to vrserver or vrcompositor.");
		break;

	case vr::VRInitError_IPC_SharedStateInitFailed:
		strcpy(errorString, "HmdError_IPC_SharedStateInitFailed(302) - Shared memory with vrserver or vrcompositor could not be opened.");
		break;

	case vr::VRInitError_IPC_CompositorInitFailed:
		strcpy(errorString, "HmdError_IPC_CompositorInitFailed(303) - OpenVR was unable to start vrcompositor.");
		break;

	case vr::VRInitError_IPC_MutexInitFailed:
		strcpy(errorString, "HmdError_IPC_MutexInitFailed(304) - OpenVR was unable to create a mutex to communicate with vrcompositor.");
		break;

	case vr::VRInitError_VendorSpecific_UnableToConnectToOculusRuntime:
		strcpy(errorString, "HmdError_VendorSpecific_UnableToConnectToOculusRuntime(1000) - The connection to the Oculus runtime failed for an unknown reason.");
		break;

	case vr::VRInitError_Steam_SteamInstallationNotFound:
		strcpy(errorString, "HmdError_Steam_SteamInstallationNotFound(2000) - This error is not currently used.)");
		break;

	default:
		strcpy(errorString, "Unknown error!");
		break;
	}
}


static void Boot(int argc, char** argv)
{
	SDL_SetAppMetadata(GAME_FULL_NAME, GAME_VERSION, GAME_IDENTIFIER);
#if _DEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#endif

	// Start our "machine"
	Pomme::Init();

	// Find path to game data folder
	const char* executablePath = argc > 0 ? argv[0] : NULL;
	fs::path dataPath = FindGameData(executablePath);

	// Load game prefs before starting
	LoadPrefs();

retryVideo:
	// Initialize SDL video subsystem
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Init OpenVR
	vr::HmdError peError = vr::VRInitError_None;
	gIVRSystem = vr::VR_Init(&peError, vr::VRApplication_Scene);
	if(peError != vr::VRInitError_None)
	{
		char errorMsg[255];
		GetIVRErrorString(errorMsg, peError);

		throw std::runtime_error(errorMsg);
	}

	if(!vr::VRCompositor())
	{
		throw std::runtime_error("Compositor initialization failed!");
	}

	// Init SteamVR Input
	vrcpp_initSteamVRInput();

	// Get OpenVR render target sizes
	gIVRSystem->GetRecommendedRenderTargetSize(&vrInfoHMD.gEyeTargetWidth, &vrInfoHMD.gEyeTargetHeight);


	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	gCurrentAntialiasingLevel = gGamePrefs.antialiasingLevel;
	if (gCurrentAntialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gCurrentAntialiasingLevel);
	}

	gSDLWindow = SDL_CreateWindow(
		GAME_FULL_NAME " " GAME_VERSION,
		vrInfoHMD.gEyeTargetWidth,
		vrInfoHMD.gEyeTargetHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	if (!gSDLWindow)
	{
		if (gCurrentAntialiasingLevel != 0)
		{
			SDL_Log("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retryVideo;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Init gamepad subsystem
	SDL_Init(SDL_INIT_GAMEPAD);
	auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
	if (-1 == SDL_AddGamepadMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, GAME_FULL_NAME, "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
	}
}

static void Shutdown()
{
	// Always restore the user's mouse acceleration before exiting.
	SetMacLinearMouse(false);

	vr::VR_Shutdown();
	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = NULL;
	}

	SDL_Quit();
}

int main(int argc, char** argv)
{
	bool success = true;
	std::string uncaught = "";

	try
	{
		Boot(argc, argv);
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)
	// In release builds, catch anything that might be thrown by GameMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		success = false;
		uncaught = ex.what();
	}
	catch (...)						// Last-resort catch
	{
		success = false;
		uncaught = "unknown";
	}
#endif

	Shutdown();

	if (!success)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Uncaught exception: %s", uncaught.c_str());
		SDL_ShowSimpleMessageBox(0, GAME_FULL_NAME, uncaught.c_str(), nullptr);
	}

	return success ? 0 : 1;
}
