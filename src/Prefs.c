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
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Prefs.h"
#include "plasmastorm.h"
#include "safeMalloc.h"
#include "utils.h"
#include "windows.h"


/** *********************************************************************
 ** Module globals and consts.
 **/
FLAGS Flags;
FLAGS OldFlags;
FLAGS DefaultFlags;

#define MAX_FILENAME_LENGTH 4096
char mPrefsFileName[MAX_FILENAME_LENGTH];


/** *********************************************************************
 ** This method ...
 **/
void initPrefsModule(int argc, char* argv[]) {
    setAllPrefDefaultValues();
    setAllPrefsFromDefaultValues();

    getPrefsFromLocalStorage();

    updatePrefsWithRuntimeValues(argc, argv);
}

/** *********************************************************************
 ** This method sets Pref default values.
 **/
void setAllPrefDefaultValues() {
    DefaultFlags.shutdownRequested = 0;
    DefaultFlags.mHideMenu = 0;
    DefaultFlags.mHaveFlagsChanged = 0;

    DefaultFlags.Language = strdup("sys");

    DefaultFlags.ShowStormItems = 1;
    DefaultFlags.StormItemColor1 = strdup("white");
    DefaultFlags.StormItemColor2 = strdup("cyan");
    DefaultFlags.ShapeSizeFactor = 8;
    DefaultFlags.StormItemSpeedFactor = 100;
    DefaultFlags.StormItemCountMax = 300;
    DefaultFlags.StormSaturationFactor = 100;

    DefaultFlags.ShowStars = 1;
    DefaultFlags.MaxStarCount = 100;

    DefaultFlags.ShowWind = 1;
    DefaultFlags.WhirlFactor = 100;
    DefaultFlags.WhirlTimer = 30;

    DefaultFlags.KeepFallenOnWindows = 1;
    DefaultFlags.MaxWindowFallenDepth = 30;
    DefaultFlags.WindowFallenTopOffset = 0;

    DefaultFlags.KeepFallenOnDesktop = 1;
    DefaultFlags.MaxDesktopFallenDepth = 50;
    DefaultFlags.DesktopFallenTopOffset = 0;

    DefaultFlags.ShowBlowoff = 1;
    DefaultFlags.BlowOffFactor = 40;

    DefaultFlags.CpuLoad = 100;
    DefaultFlags.Transparency = 0;
    DefaultFlags.Scale = 100;
    DefaultFlags.AllWorkspaces = 1;
    DefaultFlags.ComboStormShape = 0;
}

/** *********************************************************************
 ** This method ...
 **/
void setAllPrefsFromDefaultValues() {
    Flags.shutdownRequested = DefaultFlags.shutdownRequested;
    Flags.mHideMenu = DefaultFlags.mHideMenu;
    Flags.mHaveFlagsChanged = DefaultFlags.mHaveFlagsChanged;

    free(Flags.Language);
    Flags.Language = strdup(DefaultFlags.Language);

    Flags.ShowStormItems = DefaultFlags.ShowStormItems;
    free(Flags.StormItemColor1);
    Flags.StormItemColor1 = strdup(DefaultFlags.StormItemColor1);
    free(Flags.StormItemColor2);
    Flags.StormItemColor2 = strdup(DefaultFlags.StormItemColor2);
    Flags.ShapeSizeFactor = DefaultFlags.ShapeSizeFactor;
    Flags.StormItemSpeedFactor = DefaultFlags.StormItemSpeedFactor;
    Flags.StormItemCountMax = DefaultFlags.StormItemCountMax;
    Flags.StormSaturationFactor = DefaultFlags.StormSaturationFactor;

    Flags.ShowStars = DefaultFlags.ShowStars;
    Flags.MaxStarCount = DefaultFlags.MaxStarCount;

    Flags.ShowWind = DefaultFlags.ShowWind;
    Flags.WhirlFactor = DefaultFlags.WhirlFactor;
    Flags.WhirlTimer = DefaultFlags.WhirlTimer;
    Flags.ShowBlowoff = DefaultFlags.ShowBlowoff;
    Flags.BlowOffFactor = DefaultFlags.BlowOffFactor;

    Flags.KeepFallenOnWindows = DefaultFlags.KeepFallenOnWindows;
    Flags.WindowFallenTopOffset =
        DefaultFlags.WindowFallenTopOffset;
    Flags.MaxWindowFallenDepth =
        DefaultFlags.MaxWindowFallenDepth;
    Flags.KeepFallenOnDesktop = DefaultFlags.KeepFallenOnDesktop;
    Flags.MaxDesktopFallenDepth =
        DefaultFlags.MaxDesktopFallenDepth;
    Flags.DesktopFallenTopOffset =
        DefaultFlags.DesktopFallenTopOffset;

    Flags.CpuLoad = DefaultFlags.CpuLoad;
    Flags.Transparency = DefaultFlags.Transparency;
    Flags.Scale = DefaultFlags.Scale;
    Flags.AllWorkspaces = DefaultFlags.AllWorkspaces;
    Flags.ComboStormShape = DefaultFlags.ComboStormShape;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
/** *********************************************************************
 ** This method ...
 **/
void getPrefsFromLocalStorage() {
    const int MAX_INPUT_STRING_LENGTH = 255;

    char inputPrefName[MAX_INPUT_STRING_LENGTH];
    char inputPrefValue[MAX_INPUT_STRING_LENGTH];

    // If none, initialize the empty file & done.
    FILE* prefsFile = fopen(getPrefsFileName(), "r");
    if (prefsFile == NULL) {
        writePrefstoLocalStorage();
        return;
    }

    // Read & ignore Warning line # 1 & 2.
    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);

    // Get Pref name & pref value.
    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    inputPrefValue[strlen(inputPrefValue) > 0 ?
        strlen(inputPrefValue) - 1 : 0] = (char) 0;

    free(Flags.Language);
    Flags.Language = strdup(inputPrefValue);

    // Get Pref name & pref value.
    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ShowStormItems = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    inputPrefValue[strlen(inputPrefValue) > 0 ?
        strlen(inputPrefValue) - 1 : 0] = (char) 0;

    free(Flags.StormItemColor1);
    Flags.StormItemColor1 = strdup(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    inputPrefValue[strlen(inputPrefValue) > 0 ?
        strlen(inputPrefValue) - 1 : 0] = (char) 0;

    free(Flags.StormItemColor2);
    Flags.StormItemColor2 = strdup(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ShapeSizeFactor = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.StormItemSpeedFactor = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.StormItemCountMax = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.StormSaturationFactor = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ShowStars = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.MaxStarCount = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ShowWind = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.WhirlFactor = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.WhirlTimer = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ShowBlowoff = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.BlowOffFactor = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.KeepFallenOnWindows = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.MaxWindowFallenDepth = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.WindowFallenTopOffset = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.KeepFallenOnDesktop = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.MaxDesktopFallenDepth = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.DesktopFallenTopOffset = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.CpuLoad = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.Transparency = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.Scale = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.AllWorkspaces = atoi(inputPrefValue);

    fgets(inputPrefName, MAX_INPUT_STRING_LENGTH, prefsFile);
    fgets(inputPrefValue, MAX_INPUT_STRING_LENGTH, prefsFile);
    Flags.ComboStormShape = atoi(inputPrefValue);

    // Close file & exit.
    fclose(prefsFile);
}
#pragma GCC diagnostic pop

/** *********************************************************************
 ** This method ...
 **/
void writePrefstoLocalStorage() {
    FILE* prefsFile = fopen(getPrefsFileName(), "w");
    if (prefsFile == NULL) {
        return;
    }

    fprintf(prefsFile, "%s  %s\n", "GENERATED FILE - ", "Do Not Modify");
    fprintf(prefsFile, "%s  %s\n", "**************   ", "*************");

    fprintf(prefsFile, "%s\n%s\n", "Language",
        Flags.Language);

    fprintf(prefsFile, "%s\n%d\n", "ShowStormItems",
        Flags.ShowStormItems);
    fprintf(prefsFile, "%s\n%s\n", "StormItemColor1",
        Flags.StormItemColor1);
    fprintf(prefsFile, "%s\n%s\n", "StormItemColor2",
        Flags.StormItemColor2);
    fprintf(prefsFile, "%s\n%d\n", "ShapeSizeFactor",
        Flags.ShapeSizeFactor);
    fprintf(prefsFile, "%s\n%d\n", "StormItemSpeedFactor",
        Flags.StormItemSpeedFactor);
    fprintf(prefsFile, "%s\n%d\n", "StormItemCountMax",
        Flags.StormItemCountMax);
    fprintf(prefsFile, "%s\n%d\n", "StormSaturationFactor",
        Flags.StormSaturationFactor);

    fprintf(prefsFile, "%s\n%d\n", "ShowStars",
        Flags.ShowStars);
    fprintf(prefsFile, "%s\n%d\n", "MaxStarCount",
        Flags.MaxStarCount);

    fprintf(prefsFile, "%s\n%d\n", "ShowWind",
        Flags.ShowWind);
    fprintf(prefsFile, "%s\n%d\n", "WhirlFactor",
        Flags.WhirlFactor);
    fprintf(prefsFile, "%s\n%d\n", "WhirlTimer",
        Flags.WhirlTimer);
    fprintf(prefsFile, "%s\n%d\n", "ShowBlowoff",
        Flags.ShowBlowoff);
    fprintf(prefsFile, "%s\n%d\n", "BlowOffFactor",
        Flags.BlowOffFactor);

    fprintf(prefsFile, "%s\n%d\n", "KeepFallenOnWindows",
        Flags.KeepFallenOnWindows);
    fprintf(prefsFile, "%s\n%d\n", "MaxWindowFallenDepth",
        Flags.MaxWindowFallenDepth);
    fprintf(prefsFile, "%s\n%d\n", "WindowFallenTopOffset",
        Flags.WindowFallenTopOffset);
    fprintf(prefsFile, "%s\n%d\n", "KeepFallenOnDesktop",
        Flags.KeepFallenOnDesktop);
    fprintf(prefsFile, "%s\n%d\n", "MaxDesktopFallenDepth",
        Flags.MaxDesktopFallenDepth);
    fprintf(prefsFile, "%s\n%d\n", "DesktopFallenTopOffset",
        Flags.DesktopFallenTopOffset);

    fprintf(prefsFile, "%s\n%d\n", "CpuLoad",
        Flags.CpuLoad);
    fprintf(prefsFile, "%s\n%d\n", "Transparency",
        Flags.Transparency);
    fprintf(prefsFile, "%s\n%d\n", "Scale",
        Flags.Scale);
    fprintf(prefsFile, "%s\n%d\n", "AllWorkspaces",
        Flags.AllWorkspaces);
    fprintf(prefsFile, "%s\n%d\n", "ComboStormShape",
        Flags.ComboStormShape);

    fclose(prefsFile);
}

/** *********************************************************************
 ** This method ...
 **/
void updatePrefsWithRuntimeValues(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-hidemenu")) {
            Flags.mHideMenu = 1;
        }
    }
}

/** *********************************************************************
 ** String method helpers.
 **/
char* getPrefsFileName() {
    mPrefsFileName[0] = '\0';

    strcat(mPrefsFileName, getenv("HOME"));
    strcat(mPrefsFileName, "/.plasmastormrc");

    return mPrefsFileName;
}

long int getIntFromString(char* stringValue) {
    return strtol(stringValue, NULL, 0);
}

long int getPositiveIntFromString(char* stringValue) {
    const int intValue = getIntFromString(stringValue);
    return (intValue < 0) ?
        0 : intValue;
}
