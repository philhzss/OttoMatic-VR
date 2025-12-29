// OTTO MATIC ENTRY POINT
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "version.h"
#include "game.h"

#include "openvr.h"

#include <iostream>
#include <cstring>
#include <windows.h>

vr::IVRSystem *gIVRSystem;
float gIpdScale = 100.0f; // A larger number makes the world appear smaller

#if __APPLE__
#include "killmacmouseacceleration.h"
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	SDL_Window* gSDLWindow;

	CommandLineOptions gCommandLine;

/*
	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if _WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif
*/

	int GameMain(void);
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System" / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);

	if (resFileRefNum == -1)
		throw std::runtime_error("Data folder not found.");

	UseResFile(resFileRefNum);

	return dataPath;
}

static const char* GetWindowTitle()
{
	static char windowTitle[256];
	snprintf(windowTitle, sizeof(windowTitle), "Otto Matic %s", PROJECT_VERSION);
	return windowTitle;
}

void ParseCommandLine(int argc, const char** argv)
{
	memset(&gCommandLine, 0, sizeof(gCommandLine));
	gCommandLine.vsync = 1;

	for (int i = 1; i < argc; i++)
	{
		std::string argument = argv[i];

		if (argument == "--skip-fluff")
			gCommandLine.skipFluff = true;
		else if (argument == "--no-vsync")
			gCommandLine.vsync = 0;
		else if (argument == "--vsync")
			gCommandLine.vsync = 1;
		else if (argument == "--adaptive-vsync")
			gCommandLine.vsync = -1;
		else if (argument == "--fullscreen-resolution")
		{
			GAME_ASSERT_MESSAGE(i + 2 < argc, "fullscreen width & height unspecified");
			gCommandLine.fullscreenWidth = atoi(argv[i + 1]);
			gCommandLine.fullscreenHeight = atoi(argv[i + 2]);
			i += 2;
		}
		else if (argument == "--fullscreen-refresh-rate")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "fullscreen refresh rate unspecified");
			gCommandLine.fullscreenRefreshRate = atoi(argv[i + 1]);
			i += 1;
		}
	}
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

int Boot(int argc, const char** argv)
{
	SetProcessDPIAware();
	ParseCommandLine(argc, argv);

	// Start our "machine"
	Pomme::Init();

	// Load game preferences
	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);

retry:
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	fs::path dataPath = FindGameData();

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

	
	if (gGamePrefs.preferredDisplay >= SDL_GetNumVideoDisplays())
		gGamePrefs.preferredDisplay = 0;

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	if (gGamePrefs.antialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gGamePrefs.antialiasingLevel);
	}

	
	gSDLWindow = SDL_CreateWindow(
			GetWindowTitle(),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
			vrInfoHMD.gEyeTargetWidth,
			vrInfoHMD.gEyeTargetHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

	if (!gSDLWindow)
	{
		if (gGamePrefs.antialiasingLevel != 0)
		{
			printf("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...\n");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retry;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Otto Matic", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}

	// Start the game
	try
	{
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	vr::VR_Shutdown();
	Pomme::Shutdown();

	SDL_DestroyWindow(gSDLWindow);
	gSDLWindow = nullptr;

	return 0;
}

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

#if _DEBUG
	// In debug builds, if CommonMain throws, don't catch.
	// This way, it's easier to get a clean stack trace.
	returnCode = Boot(argc, const_cast<const char **>(argv));
#else
	// In release builds, catch anything that might be thrown by Boot
	// so we can show an error dialog to the user.
	try
	{
		returnCode = Boot(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)		// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = ex.what();
		showFinalErrorMessage = true;
	}
	catch (...)						// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = "unknown";
		showFinalErrorMessage = true;
	}
#endif

#if __APPLE__
	// Whether we failed or succeeded, always restore the user's mouse acceleration before exiting.
	// (NOTE: in debug builds, we might not get here because we don't catch what CommonMain throws.)
	RestoreMacMouseAcceleration();
#endif

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Otto Matic", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}
