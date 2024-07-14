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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "ClockHelper.h"
#include "Prefs.h"
#include "plasmastorm.h"
#include "utils.h"
#include "Wind.h"
#include "windows.h"


/** *********************************************************************
 ** Module globals and consts.
 **/
const int WHIRL_EVENT_PROBABILITY = 150 * .01;
const int MINIMUM_WIND_WHIRL_COUNTDOWN_TIMER_VALUE = 3;

bool mShortWindInitialized = false;
double mPreviousShortWindStartTime;


/** *********************************************************************
 ** This method ...
 **/
void initWindModule() {
    setWindWhirlValue();
    setWindWhirlTimers();

    addMethodToMainloop(PRIORITY_DEFAULT,
        DO_LONG_WIND_EVENT_TIME, doLongWindEvent);
    addMethodToMainloop(PRIORITY_DEFAULT,
        DO_SHORT_WIND_EVENT_TIME, doShortWindEvent);
}

/** *********************************************************************
 ** This method ...
 **/
void respondToWindSettingsChanges() {
    //UIDO(ShowWind,
    if (Flags.ShowWind != OldFlags.ShowWind) {
        OldFlags.ShowWind = Flags.ShowWind;
        mGlobal.Wind = 0;
        mGlobal.NewWind = 0;
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(WhirlFactor,
    if (Flags.WhirlFactor != OldFlags.WhirlFactor) {
        OldFlags.WhirlFactor = Flags.WhirlFactor;
        setWindWhirlValue();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(WhirlTimer,
    if (Flags.WhirlTimer != OldFlags.WhirlTimer) {
        OldFlags.WhirlTimer = Flags.WhirlTimer;
        setWindWhirlTimers();
        Flags.mHaveFlagsChanged++;
    }
}

/** *********************************************************************
 ** This method sets the wind whirl value.
 **/
void setWindWhirlValue() {
    mGlobal.windWhirlValue = Flags.WhirlFactor *
        WHIRL_EVENT_PROBABILITY;
}

/** *********************************************************************
 ** This method resets wind whirl timer and timer start from user
 ** control.
 **/
void setWindWhirlTimers() {
    mGlobal.windWhirlTimerStart =
        Flags.WhirlTimer > MINIMUM_WIND_WHIRL_COUNTDOWN_TIMER_VALUE ?
        Flags.WhirlTimer : MINIMUM_WIND_WHIRL_COUNTDOWN_TIMER_VALUE;

    mGlobal.windWhirlTimer = mGlobal.windWhirlTimerStart;
}

/** *********************************************************************
 ** This method performs a short wind event.
 **/
int doShortWindEvent() {
    if (Flags.shutdownRequested) {
        return false;
    }

    if (!WorkspaceActive()) {
        return true;
    }
    if (!Flags.ShowWind) {
        return true;
    }

    // On the average, this function will do
    // somethingafter WhirlTimer secs.
    const double timeNow = wallclock();
    if (!mShortWindInitialized) {
        mPreviousShortWindStartTime = timeNow;
        mShortWindInitialized = true;
        return true;
    }

    // Delay event state time change until magic.
    if ((timeNow - mPreviousShortWindStartTime) <
        mGlobal.windWhirlTimer * 2 * drand48()) {
        return true;
    }
    mPreviousShortWindStartTime = timeNow;

    // Now for some of Rick's magic.
    if (drand48() > 0.65) {
        if (drand48() > 0.4) {
            mGlobal.Direction = 1;
        } else {
            mGlobal.Direction = -1;
        }
        mGlobal.Wind = 2;
        mGlobal.windWhirlTimer = 5;
        return true;
    }

    if (mGlobal.Wind == 2) {
        mGlobal.Wind = 1;
        mGlobal.windWhirlTimer = 3;
        return true;
    }

    mGlobal.Wind = 0;
    mGlobal.windWhirlTimer = mGlobal.windWhirlTimerStart;

    return true;
}

/** *********************************************************************
 ** This method performs a long wind event.
 **
 ** The speed of wind is pixels/second at steady Wind, eventually
 ** all storm items get this speed.
 **/
int doLongWindEvent() {
    if (Flags.shutdownRequested) {
        return false;
    }

    if (!WorkspaceActive()) {
        return true;
    }
    if (!Flags.ShowWind) {
        return true;
    }

    switch (mGlobal.Wind) {
        case (2):
            mGlobal.NewWind = mGlobal.Direction *
                mGlobal.windWhirlValue * 1.2;
            break;

        case (1):
            mGlobal.NewWind = mGlobal.Direction *
                mGlobal.windWhirlValue * .06;
            break;

        case (0):
        default:
            const float newWindDelta =
                (float) (drand48() * mGlobal.windWhirlValue) -
                 mGlobal.windWhirlValue / 2;

            mGlobal.NewWind += newWindDelta;
            if (mGlobal.NewWind > mGlobal.WindMax) {
                mGlobal.NewWind = mGlobal.WindMax;
            }
            if (mGlobal.NewWind < -mGlobal.WindMax) {
                mGlobal.NewWind = -mGlobal.WindMax;
            }
            break;
    }

    return true;
}
