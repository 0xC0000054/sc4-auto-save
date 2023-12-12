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

#include "cGZAutoSaveService.h"
#include "cIGZApp.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Simulator.h"
#include <string>
#include <Windows.h>

static constexpr uint32_t kAutoSaveServiceID = 0x3fa8cbb9;

static constexpr uint32_t GZIID_cISC4App = 0x26ce01c0;

namespace
{
#ifdef _DEBUG
	void PrintLineToDebugOutput(const char* line)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);

		char buffer[1024]{};

		std::snprintf(buffer,
			sizeof(buffer),
			"[%hu:%hu:%hu.%hu] %s",
			time.wHour,
			time.wMinute,
			time.wSecond,
			time.wMilliseconds,
			line);

		OutputDebugStringA(buffer);
		OutputDebugStringA("\n");
	}

	void PrintLineToDebugOutputFormatted(const char* format, ...)
	{
		char buffer[1024]{};

		va_list args;
		va_start(args, format);

		std::vsnprintf(buffer, sizeof(buffer), format, args);

		va_end(args);

		PrintLineToDebugOutput(buffer);
	}	
#endif // _DEBUG
}

cGZAutoSaveService::cGZAutoSaveService()
	: ServiceBase(kAutoSaveServiceID, 1000000),	  
	  addedSystemService(false),
	  running(false),
	  saveIntervalInMinutes(15),
	  fastSave(true),
	  logSaveEvents(true),
	  autoSaveTimer(),
	  pFramework(nullptr),
	  pSC4App(nullptr)
{
}

bool cGZAutoSaveService::PostAppInit(cIGZFrameWork* pFramework, const Settings& settings)
{
	Logger& logger = Logger::GetInstance();

	bool result = false;

	if (pFramework)
	{
		this->pFramework = pFramework;

		cIGZApp* pApp = pFramework->Application();

		if (pApp)
		{
			if (pApp->QueryInterface(GZIID_cISC4App, reinterpret_cast<void**>(&pSC4App)))
			{
				saveIntervalInMinutes = settings.SaveIntervalInMinutes();
				fastSave = settings.FastSave();
				logSaveEvents = settings.LogSaveEvents();

				result = Init();
			}
			else
			{
				logger.WriteLine(LogLevel::Error, "QueryInterface(GZIID_cISC4App...) returned false, not SC4?");
			}
		}
		else
		{
			logger.WriteLine(LogLevel::Error, "The cIGZApp pointer was null.");
		}
	}
	else
	{
		logger.WriteLine(LogLevel::Error, "The cIGZFrameWork pointer was null.");
	}

	return result;
}

bool cGZAutoSaveService::PreAppShutdown()
{
	bool result = Shutdown();

	pSC4App->Release();
	pSC4App = nullptr;
	pFramework = nullptr;

	return result;
}

void cGZAutoSaveService::Start()
{
	if (!running)
	{
		pFramework->AddToOnIdle(this);
		autoSaveTimer.Start();
		running = true;
	}
}

void cGZAutoSaveService::Stop()
{
	if (running)
	{
		pFramework->RemoveFromOnIdle(this);
		autoSaveTimer.Stop();
		running = false;
	}
}

bool cGZAutoSaveService::CanSaveCity() const
{
	bool canSave = false;

	if (pSC4App)
	{
		cISC4City* pCity = pSC4App->GetCity();

		if (pCity && !pCity->IsSaveDisabled())
		{
			cISC4Simulator* pSimulator = pCity->GetSimulator();

			if (pSimulator && !pSimulator->IsAnyPaused())
			{
				canSave = true;
			}
		}
	}

	return canSave;
}

bool cGZAutoSaveService::Init()
{
	if (!addedSystemService)
	{
		addedSystemService = pFramework->AddSystemService(this);
	}

	return addedSystemService;
}

bool cGZAutoSaveService::Shutdown()
{
	bool result = true;

	if (addedSystemService)
	{
		Stop();
		pFramework->RemoveSystemService(this);
		addedSystemService = false;
	}

	return true;
}

bool cGZAutoSaveService::OnIdle(uint32_t unknown1)
{
	int64_t elapsedMinutes = autoSaveTimer.ElapsedMinutes();

	if (elapsedMinutes >= saveIntervalInMinutes)
	{
		if (CanSaveCity())
		{
			const char* status = nullptr;
#ifdef _DEBUG
			PrintLineToDebugOutputFormatted("Saving city, FastSave=%s", fastSave ? "true" : "false");
#endif // _DEBUG
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

			if (logSaveEvents)
			{
				Logger::GetInstance().WriteLine(LogLevel::Info, status);
			}

			autoSaveTimer.Restart();
		}
	}

	return true;
}
