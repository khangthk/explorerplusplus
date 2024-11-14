// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// These values are used when loading and saving data and shouldn't be changed.
// clang-format off
BETTER_ENUM(WindowShowState, int,
	Normal = 0,
	Minimized = 1,
	Maximized = 2
)
// clang-format on

struct WindowStorageData
{
	RECT bounds;
	WindowShowState showState;
};

WindowShowState NativeShowStateToShowState(int nativeShowState);
int ShowStateToNativeShowState(WindowShowState showState);