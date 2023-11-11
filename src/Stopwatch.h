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

#include <stdint.h>

class Stopwatch
{
public:

	Stopwatch() noexcept;

	int64_t ElapsedMilliseconds() const;

	int64_t ElapsedSeconds() const;

	int64_t ElapsedMinutes() const;

	bool IsRunning() const;

	void Reset();

	void Restart();

	void Start();

	void Stop();

private:

	int64_t GetElapsedTicks() const;

	const int64_t tickFrequency;
	int64_t elapsed;
	int64_t startTimeStamp;
	bool isRunning;
};

