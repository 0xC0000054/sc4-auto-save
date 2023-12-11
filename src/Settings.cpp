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
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"

namespace pt = boost::property_tree;

Settings::Settings()
	: saveIntervalInMinutes(15),
	  fastSave(false),
	  ignoreTimePaused(true),
      logSaveEvents(true)
{
}

int Settings::SaveIntervalInMinutes() const
{
	return saveIntervalInMinutes;
}

bool Settings::FastSave() const
{
	return fastSave;
}

bool Settings::IgnoreTimePaused() const
{
	return ignoreTimePaused;
}

bool Settings::LogSaveEvents() const
{
	return logSaveEvents;
}

void Settings::Load(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ifstream::in);

	if (!stream)
	{
		throw std::runtime_error("Failed to open the settings file.");
	}

	pt::ptree tree;

	pt::ini_parser::read_ini(stream, tree);

	saveIntervalInMinutes = tree.get<int>("AutoSave.IntervalInMinutes");
	fastSave = tree.get<bool>("AutoSave.FastSave");
	ignoreTimePaused = tree.get<bool>("AutoSave.IgnoreTimePaused");
	logSaveEvents = tree.get<bool>("AutoSave.LogSaveEvents");
}
