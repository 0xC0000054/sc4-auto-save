////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-auto-save, a DLL Plugin for SimCity 4
// that automatically saves a city at user-specified intervals.
//
// Copyright (c) 2023 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////

#include "Settings.h"
#include "Stopwatch.h"
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
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/filesystem.h"

static constexpr uint32_t kSC4MessageCityEstablished = 0x26D31EC4;
static constexpr uint32_t kSC4MessagePostCityInit = 0x26d31ec1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;
static constexpr uint32_t kSC4MessageSimNewDay = 0x66956814;
static constexpr uint32_t kSC4MessageSimPauseChange = 0xAA7FB7E0;
static constexpr uint32_t kSC4MessageSimHiddenPauseChange = 0x4A7FB7E2;
static constexpr uint32_t kSC4MessageSimEmergencyPauseChange = 0x4A7FB807;

static constexpr uint32_t kAutoSavePluginDirectorID = 0xb0bd667d;

static constexpr int kMinimumSaveIntervalInMinutes = 1;
static constexpr int kMaximumSaveIntervalInMinutes = 120;

static constexpr std::string_view PluginConfigFileName = "SC4AutoSave.ini";
static constexpr std::string_view PluginLogFileName = "SC4AutoSave.log";

class cGZAutoSaveDllDirector : public cRZMessage2COMDirector
{
public:

	cGZAutoSaveDllDirector()
		: autoSaveTimer(),
		  pauseEventCount(0),
		  cityEstablished(false),
		  settings(),
		  logFile()
	{
		std::filesystem::path dllFolder = GetDllFolderPath();

		configFilePath = dllFolder;
		configFilePath /= PluginConfigFileName;

		std::filesystem::path logFilePath = dllFolder;
		logFilePath /= PluginLogFileName;

		logFile.open(logFilePath, std::ofstream::out | std::ofstream::trunc);
		if (logFile)
		{
			logFile << "SC4AutoSave v" PLUGIN_VERSION_STR << std::endl;
		}
	}

	uint32_t GetDirectorID() const
	{
		return kAutoSavePluginDirectorID;
	}

	bool CanSaveCity(cISC4App* pSC4App)
	{
		bool canSave = false;

		if (pSC4App)
		{
			cISC4City* pCity = pSC4App->GetCity();

			if (pCity && !pCity->IsSaveDisabled())
			{
				cISC4SimulatorPtr pSimulator;

				if (pSimulator && !pSimulator->IsAnyPaused())
				{
					canSave = true;
				}
			}
		}

		return canSave;
	}

	void CityEstablished()
	{
		cityEstablished = true;
		autoSaveTimer.Start();
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
				autoSaveTimer.Stop();
			}
		}
		else
		{
			if (pauseEventCount > 0)
			{
				pauseEventCount--;

				if (pauseEventCount == 0)
				{
					autoSaveTimer.Start();
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
				autoSaveTimer.Start();
			}
		}
	}

	void PreCityShutdown()
	{
		cityEstablished = false;
		autoSaveTimer.Stop();
	}

	void SimNewDay()
	{
		if (cityEstablished && autoSaveTimer.IsRunning())
		{
			int64_t elapsedMinutes = autoSaveTimer.ElapsedMinutes();

			if (elapsedMinutes >= settings.SaveIntervalInMinutes())
			{
				cISC4AppPtr pSC4App;

				if (CanSaveCity(pSC4App))
				{
#ifdef _DEBUG
					PrintLineToDebugOutput("Saving city...");
#endif // _DEBUG

					const char* status = nullptr;
					bool fastSave = settings.FastSave();

					if (pSC4App->SaveCity(fastSave))
					{
						status = "City saved.";
					}
					else
					{
						status = "The games's SaveCity command failed.";
					}

#ifdef _DEBUG
					PrintLineToDebugOutput(status);
#endif // _DEBUG

					if (settings.LogSaveEvents() && logFile)
					{
						SYSTEMTIME time;
						GetSystemTime(&time);

						char buffer[1024]{};

						int charsWritten = snprintf(buffer,
							sizeof(buffer),
							"[%hu:%hu:%hu.%hu] %s",
							time.wHour,
							time.wMinute,
							time.wSecond,
							time.wMilliseconds,
							status);

						if (charsWritten > 0)
						{
							logFile << buffer << std::endl;
						}
					}

					autoSaveTimer.Restart();
				}
			}
		}
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
		case kSC4MessageSimNewDay:
			SimNewDay();
			break;
		case kSC4MessageSimPauseChange:
		case kSC4MessageSimHiddenPauseChange:
		case kSC4MessageSimEmergencyPauseChange:
			GamePause(pStandardMsg);
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
				return true;
			}
			else if (saveInterval > kMaximumSaveIntervalInMinutes)
			{
				char buffer[1024]{};

				std::snprintf(buffer,
							  sizeof(buffer),
							  "The save interval is greater than %d minute(s).",
							  kMaximumSaveIntervalInMinutes);

				MessageBoxA(nullptr, buffer, "SC4AutoSave - Error when loading settings", MB_OK | MB_ICONERROR);
				return true;
			}
		}
		catch (const std::exception& ex)
		{
			MessageBoxA(nullptr, ex.what(), "SC4AutoSave - Error when loading settings", MB_OK | MB_ICONERROR);
			return true;
		}

		cIGZMessageServer2Ptr pMsgServ;
		if (pMsgServ)
		{
			std::vector<uint32_t> requiredNotifications;
			requiredNotifications.push_back(kSC4MessageCityEstablished);
			requiredNotifications.push_back(kSC4MessagePostCityInit);
			requiredNotifications.push_back(kSC4MessagePreCityShutdown);
			requiredNotifications.push_back(kSC4MessageSimNewDay);

			if (settings.IgnoreTimePaused())
			{
				requiredNotifications.push_back(kSC4MessageSimPauseChange);
				requiredNotifications.push_back(kSC4MessageSimHiddenPauseChange);
				requiredNotifications.push_back(kSC4MessageSimEmergencyPauseChange);
			}

			for (uint32_t messageID : requiredNotifications)
			{
				if (!pMsgServ->AddNotification(this, messageID))
				{
					MessageBoxA(nullptr, "Failed to subscribe to the required notifications.", "SC4AutoSave", MB_OK | MB_ICONERROR);
					return true;
				}
			}
		}
		else
		{
			MessageBoxA(nullptr, "Failed to subscribe to the required notifications.", "SC4AutoSave", MB_OK | MB_ICONERROR);
			return true;
		}

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

#ifdef _DEBUG
	void PrintLineToDebugOutput(const char* line)
	{
		SYSTEMTIME time;
		GetSystemTime(&time);

		char buffer[1024]{};

		std::snprintf(buffer,
			sizeof(buffer),
			"[%hu:%hu:%hu.%hu] %s\n",
			time.wHour,
			time.wMinute,
			time.wSecond,
			time.wMilliseconds,
			line);

		OutputDebugStringA(line);
	}
#endif // _DEBUG


	Stopwatch autoSaveTimer;
	int pauseEventCount;
	bool cityEstablished;
	Settings settings;
	std::filesystem::path configFilePath;
	std::ofstream logFile;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static cGZAutoSaveDllDirector sDirector;
	return &sDirector;
}