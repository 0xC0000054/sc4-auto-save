////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-auto-save, a DLL Plugin for SimCity 4
// that automatically saves a city at user-specified intervals.
//
// Copyright (c) 2023, 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////

#include "cGZAutoSaveService.h"
#include "Logger.h"
#include "Settings.h"
#include "version.h"
#include "cIGZFrameWork.h"
#include "cIGZApp.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Simulator.h"
#include "cIGZMessageServer2.h"
#include "cIGZMessageTarget.h"
#include "cIGZMessageTarget2.h"
#include "cIGZString.h"
#include "cRZMessage2COMDirector.h"
#include "cRZMessage2Standard.h"
#include "cRZBaseString.h"
#include "GZServPtrs.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/filesystem.h"

static constexpr uint32_t kSC4MessageCityEstablished = 0x26D31EC4;
static constexpr uint32_t kSC4MessagePostCityInit = 0x26d31ec1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;
static constexpr uint32_t kSC4MessageSimPauseChange = 0xAA7FB7E0;
static constexpr uint32_t kSC4MessageSimHiddenPauseChange = 0x4A7FB7E2;
static constexpr uint32_t kSC4MessageSimEmergencyPauseChange = 0x4A7FB807;
static constexpr uint32_t kMessageTypeAppGainLoseFocus = 0x4348B111;

static constexpr uint32_t kAutoSavePluginDirectorID = 0xb0bd667d;

static constexpr int kMinimumSaveIntervalInMinutes = 1;
static constexpr int kMaximumSaveIntervalInMinutes = 120;

static constexpr std::string_view PluginConfigFileName = "SC4AutoSave.ini";
static constexpr std::string_view PluginLogFileName = "SC4AutoSave.log";

class cGZAutoSaveDllDirector : public cRZMessage2COMDirector
{
public:

	cGZAutoSaveDllDirector()
		: autoSaveService(),
		  loseFocusEventCount(0),
		  pauseEventCount(0),
		  cityEstablished(false),
		  settings()
	{
		std::filesystem::path dllFolder = GetDllFolderPath();

		configFilePath = dllFolder;
		configFilePath /= PluginConfigFileName;

		std::filesystem::path logFilePath = dllFolder;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();

		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("SC4AutoSave v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kAutoSavePluginDirectorID;
	}

	void CityEstablished()
	{
		cityEstablished = true;
		autoSaveService.StartTimer();
	}

	void AppGainLoseFocus(cIGZMessage2Standard* pStandardMsg)
	{
		if (!cityEstablished)
		{
			return;
		}

		bool hasFocus = pStandardMsg->GetData1() != 0;

		if (hasFocus)
		{
			if (loseFocusEventCount > 0)
			{
				loseFocusEventCount--;

				if (loseFocusEventCount == 0)
				{
					autoSaveService.SetAppHasFocus(true);
				}
			}
		}
		else
		{
			loseFocusEventCount++;

			if (loseFocusEventCount == 1)
			{
				autoSaveService.SetAppHasFocus(false);
			}
		}
	}

	void GamePause(cIGZMessage2Standard* pStandardMsg)
	{
		if (!cityEstablished)
		{
			return;
		}

		bool pauseActive = pStandardMsg->GetData1() != 0;

		if (pauseActive)
		{
			pauseEventCount++;

			if (pauseEventCount == 1)
			{
				// When the game is paused we either stop the auto save timer
				// or leave it running and remove the auto save service from
				// the game's OnIdle callback.
				// Stopping the auto save timer will also remove the auto save
				// service from the game's OnIdle callback
				//
				// This prevents the game from wasting CPU on the auto save timer
				// checks. We never save a city when the game is paused.

				if (settings.IgnoreTimePaused())
				{
					autoSaveService.StopTimer();
				}
				else
				{
					autoSaveService.RemoveFromOnIdle();
				}
			}
		}
		else
		{
			if (pauseEventCount > 0)
			{
				pauseEventCount--;

				if (pauseEventCount == 0)
				{
					if (settings.IgnoreTimePaused())
					{
						autoSaveService.StartTimer();
					}
					else
					{
						autoSaveService.AddToOnIdle();
					}
				}
			}
		}
	}

	void PostCityInit(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = reinterpret_cast<cISC4City*>(pStandardMsg->GetIGZUnknown());

		if (pCity)
		{
			// We only enable auto-save after a city has been established.
			// There is no point in running it before then.
			if (pCity->GetEstablished())
			{
				cityEstablished = true;
				autoSaveService.StartTimer();
			}
		}
	}

	void PreCityShutdown()
	{
		cityEstablished = false;
		autoSaveService.StopTimer();
	}

	bool DoMessage(cIGZMessage2* pMessage)
	{
		cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMessage);
		uint32_t dwType = pMessage->GetType();

		switch (dwType)
		{
		case kSC4MessagePostCityInit:
			PostCityInit(pStandardMsg);
			break;
		case kSC4MessagePreCityShutdown:
			PreCityShutdown();
			break;
		case kSC4MessageCityEstablished:
			CityEstablished();
			break;
		case kSC4MessageSimPauseChange:
		case kSC4MessageSimHiddenPauseChange:
		case kSC4MessageSimEmergencyPauseChange:
			GamePause(pStandardMsg);
			break;
		case kMessageTypeAppGainLoseFocus:
			AppGainLoseFocus(pStandardMsg);
			break;
		}

		return true;
	}

	bool PostAppInit()
	{
		try
		{
			settings.Load(configFilePath);

			int saveInterval = settings.SaveIntervalInMinutes();

			if (saveInterval < kMinimumSaveIntervalInMinutes)
			{
				char buffer[1024]{};

				std::snprintf(buffer,
						      sizeof(buffer),
							  "The save interval is less than %d minute(s).",
							  kMinimumSaveIntervalInMinutes);

				MessageBoxA(nullptr, buffer, "SC4AutoSave - Error when loading settings", MB_OK | MB_ICONERROR);
				return false;
			}
			else if (saveInterval > kMaximumSaveIntervalInMinutes)
			{
				char buffer[1024]{};

				std::snprintf(buffer,
							  sizeof(buffer),
							  "The save interval is greater than %d minute(s).",
							  kMaximumSaveIntervalInMinutes);

				MessageBoxA(nullptr, buffer, "SC4AutoSave - Error when loading settings", MB_OK | MB_ICONERROR);
				return false;
			}
		}
		catch (const std::exception& ex)
		{
			MessageBoxA(nullptr, ex.what(), "SC4AutoSave - Error when loading settings", MB_OK | MB_ICONERROR);
			return false;
		}

		cIGZMessageServer2Ptr pMsgServ;
		if (pMsgServ)
		{
			std::vector<uint32_t> requiredNotifications;
			requiredNotifications.push_back(kSC4MessageCityEstablished);
			requiredNotifications.push_back(kSC4MessagePostCityInit);
			requiredNotifications.push_back(kSC4MessagePreCityShutdown);
			requiredNotifications.push_back(kMessageTypeAppGainLoseFocus);
			requiredNotifications.push_back(kSC4MessageSimPauseChange);
			requiredNotifications.push_back(kSC4MessageSimHiddenPauseChange);
			requiredNotifications.push_back(kSC4MessageSimEmergencyPauseChange);

			for (uint32_t messageID : requiredNotifications)
			{
				if (!pMsgServ->AddNotification(this, messageID))
				{
					MessageBoxA(nullptr, "Failed to subscribe to the required notifications.", "SC4AutoSave", MB_OK | MB_ICONERROR);
					return false;
				}
			}
		}
		else
		{
			MessageBoxA(nullptr, "Failed to subscribe to the required notifications.", "SC4AutoSave", MB_OK | MB_ICONERROR);
			return false;
		}

		cIGZFrameWork* pFramework = FrameWork();

		if (!autoSaveService.PostAppInit(pFramework, settings))
		{
			MessageBoxA(nullptr, "Failed to initialize the auto save service.", "SC4AutoSave", MB_OK | MB_ICONERROR);
			return false;
		}

		return true;
	}

	bool PreAppShutdown()
	{
		autoSaveService.PreAppShutdown();
		return true;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		cIGZFrameWork* const pFramework = RZGetFrameWork();

		if (pFramework->GetState() < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}

		return true;
	}

private:

	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	cGZAutoSaveService autoSaveService;
	int pauseEventCount;
	int loseFocusEventCount;
	bool cityEstablished;
	Settings settings;
	std::filesystem::path configFilePath;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static cGZAutoSaveDllDirector sDirector;
	return &sDirector;
}