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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "Blowoff.h"
#include "Fallen.h"
#include "Prefs.h"
#include "plasmastorm.h"
#include "windows.h"
#include "utils.h"

// static int handleBlowoffEvent();


/** *********************************************************************
 ** Module globals and consts.
 **/
static int lockcounter = 0;


/** *********************************************************************
 ** This method inits all for the Blowoff module.
 **/
void initBlowoffModule() {
    addMethodToMainloop(PRIORITY_DEFAULT, DO_BLOWOFF_EVENT_TIME,
        handleBlowoffEvent);
}

/** *********************************************************************
 ** This method updates the Blowoff module based on
 ** the users settings.
 **/
void respondToBlowoffSettingsChanges() {
    //UIDO(ShowBlowoff, );
    if (Flags.ShowBlowoff != OldFlags.ShowBlowoff) {
        OldFlags.ShowBlowoff = Flags.ShowBlowoff;
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(BlowOffFactor, );
    if (Flags.BlowOffFactor != OldFlags.BlowOffFactor) {
        OldFlags.BlowOffFactor = Flags.BlowOffFactor;
        Flags.mHaveFlagsChanged++;
    }
}

/** *********************************************************************
 ** This method gets a random number for a blowoff event.
 **/
int getBlowoffEventCount() {
    return 0.04 * Flags.BlowOffFactor * drand48();
}

/** *********************************************************************
 ** This method processes Blowoff events.
 **/
int handleBlowoffEvent() {
    if (Flags.Done) {
        return false;
    }
    if (!WorkspaceActive() || !Flags.ShowBlowoff) {
        return true;
    }
    if (softLockFallenBaseSemaphore(3, &lockcounter)) {
        return true;
    }

    FallenItem *fallen = mGlobal.FallenFirst;
    while (fallen) {
        if (canFallenConsumeStormItem(fallen) &&
            Flags.ShowStormItems) {
            if (fallen->winInfo.window == 0 || (!fallen->winInfo.hidden &&
                (isFallenOnVisibleWorkspace(fallen) ||
                fallen->winInfo.sticky))) {
                updateFallenWithWind(fallen, fallen->w / 4, fallen->h / 4);
            }
        }
        fallen = fallen->next;
    }

    unlockFallenSemaphore();
    return true;
}
