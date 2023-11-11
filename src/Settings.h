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

#include <filesystem>
#include <fstream>

class Settings
{
public:

	Settings();

	int SaveIntervalInMinutes() const;

	// Fast saving skips updating the region view thumbnail.
	bool FastSave() const;

	// Will the time the game spends paused count towards the next auto-save point.
	// If this is false, the next auto-save may occur after the game resumes.
	bool IgnoreTimePaused() const;

	// The save event status will be written to the log.
	bool LogSaveEvents() const;

	void Load(const std::filesystem::path& path);

private:
	int saveIntervalInMinutes;
	bool fastSave;
	bool ignoreTimePaused;
	bool logSaveEvents;
};

