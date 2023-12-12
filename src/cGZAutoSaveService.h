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

#pragma once
#include "ServiceBase.h"
#include "Logger.h"
#include "Settings.h"
#include "Stopwatch.h"
#include "cIGZFrameWork.h"
#include "cISC4App.h"

class cGZAutoSaveService final : private ServiceBase
{
public:

	cGZAutoSaveService();

	bool PostAppInit(cIGZFrameWork* pFramework, const Settings& appSettings);

	bool PreAppShutdown();

	void Start();

	void Stop();

private:

	bool CanSaveCity() const;

	bool Init() override;

	bool Shutdown() override;

	bool OnIdle(uint32_t unknown1) override;

	bool addedSystemService;
	bool running;
	int saveIntervalInMinutes;
	bool fastSave;
	bool logSaveEvents;
	Stopwatch autoSaveTimer;
	cIGZFrameWork* pFramework;
	cISC4App* pSC4App;
};

