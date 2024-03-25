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
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "ClockHelper.h"
#include "loadmeasure.h"
#include "MainWindow.h"
#include "plasmastorm.h"
#include "Prefs.h"
#include "utils.h"


/***********************************************************
 * Module consts.
 */
#define LOAD_PRESSURE_LOW  -10
#define LOAD_PRESSURE_HIGH 10

#define EXCESSIVE_LOAD_MONITOR_TIME_PCT 1.2

#define OVER_PRESSURE_TIME_MAX \
    DO_LOAD_MONITOR_EVENT_TIME * \
    EXCESSIVE_LOAD_MONITOR_TIME_PCT

#define WARNING_COUNT_MAX 3

bool mIsSystemBusy = false;
int mLoadPressure = 0;
int mWarningCount = 0;
double mPreviousTime = 0;


/** *********************************************************************
 ** Add update method to mainloop.
 **/
void addLoadMonitorToMainloop() {
    addMethodToMainloop(PRIORITY_DEFAULT,
        DO_LOAD_MONITOR_EVENT_TIME, updateLoadMonitor);
}

/** *********************************************************************
 ** Periodically check app performance.
 **
 **    Enable or disable CSS "Busy" Style.
 **/
int updateLoadMonitor() {
    // Get elapsed time, save previous time.
    const double elapsedTime = wallclock() - mPreviousTime;
    mPreviousTime = wallclock();

    // Incr or decr mLoadPressure accordingly.
    mLoadPressure += (elapsedTime > OVER_PRESSURE_TIME_MAX) ? +1 : -1;

    // Pressure high, check / reset warnings.
    if (mLoadPressure > LOAD_PRESSURE_HIGH) {
        if (!mIsSystemBusy) {
            mIsSystemBusy = true;
            if (mWarningCount < WARNING_COUNT_MAX) {
                mWarningCount++;
            }
             addBusyStyleClass();
        }
        mLoadPressure = 0;
        return true;
    }

    // Pressure low, check / reset warnings.
    if (mLoadPressure < LOAD_PRESSURE_LOW) {
        if (mIsSystemBusy) {
            mIsSystemBusy = false;
            removeBusyStyleClass();
        }
        mLoadPressure = 0;
        return true;
    }

    // Pressure normal.
    return true;
}
