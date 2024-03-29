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

#pragma once
#include "ServiceBase.h"
#include "Logger.h"
#include "Settings.h"
#include "Stopwatch.h"
#include "cIGZFrameWork.h"
#include "cIGZWinMgr.h"
#include "cISC4App.h"
#include "cRZAutoRefCount.h"

class cGZAutoSaveService final : private ServiceBase
{
public:

	cGZAutoSaveService();

	bool PostAppInit(cIGZFrameWork* pFramework, const Settings& appSettings);

	bool PreAppShutdown();

	void StartTimer();

	void StopTimer();

	void AddToOnIdle();

	void RemoveFromOnIdle();

	void SetAppHasFocus(bool value);

private:

	bool CanSaveCity() const;

	bool Init() override;

	bool Shutdown() override;

	bool OnIdle(uint32_t unknown1) override;

	bool addedSystemService;
	bool addedToOnIdle;
	bool running;
	int saveIntervalInMinutes;
	bool fastSave;
	bool logSaveEvents;
	bool appHasFocus;
	Stopwatch autoSaveTimer;
	cRZAutoRefCount<cIGZFrameWork> pFramework;
	cRZAutoRefCount<cISC4App> pSC4App;
	cRZAutoRefCount<cIGZWinMgr> pWinMgr;
};

