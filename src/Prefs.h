/* -copyright-
#-# 
#-# plasmastorm: Storms of drifting items: snow, leaves, rain.
#-# 
#-# Copyright (C) 2024 Mark Capella
#-# 
#-# This program is free software: you can redistribute it and/or modify
#-# it under the terms of the GNU General Public License as published by
#-# the Free Software Foundation, either version 3 of the License, or
#-# (at your option) any later version.
#-# 
#-# This program is distributed in the hope that it will be useful,
#-# but WITHOUT ANY WARRANTY; without even the implied warranty of
#-# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#-# GNU General Public License for more details.
#-# 
#-# You should have received a copy of the GNU General Public License
#-# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-# 
*/
#pragma once

#include <X11/Xlib.h>

#include "safeMalloc.h"


typedef struct {

    char* Language;
    int mHideMenu;
    int mHaveFlagsChanged;
    bool shutdownRequested;

    int ShowStormItems;
    int ComboStormShape;
    char* StormItemColor1;
    char* StormItemColor2;
    int ShapeSizeFactor;
    int StormItemSpeedFactor;
    int StormItemCountMax;
    int StormSaturationFactor;

    int ShowStars;
    int MaxStarCount;

    int ShowWind;
    int WhirlFactor;
    int WhirlTimer;
    int ShowBlowoff;
    int BlowOffFactor;

    int KeepFallenOnWindows;
    int MaxWindowFallenDepth;
    int WindowFallenTopOffset;
    int KeepFallenOnDesktop;
    int MaxDesktopFallenDepth;
    int DesktopFallenTopOffset;

    int CpuLoad;
    int Transparency;
    int Scale;
    int AllWorkspaces;

} FLAGS;

extern FLAGS Flags;
extern FLAGS OldFlags;
extern FLAGS DefaultFlags;


extern void initPrefsModule(int argc, char* argv[]);

void setAllPrefDefaultValues();
void setAllPrefsFromDefaultValues();

void getPrefsFromLocalStorage();
extern void writePrefstoLocalStorage(void);

extern void updatePrefsWithRuntimeValues(
    int argc, char* argv[]);

char* getPrefsFileName();
long int getIntFromString(char*);
long int getPositiveIntFromString(char*);
