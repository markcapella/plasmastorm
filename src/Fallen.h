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

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>

#include <gtk/gtk.h>

#include "plasmastorm.h"


// Semaphore helpers.
extern void initFallenSemaphores();

// Swap semaphores.
int lockFallenSwapSemaphore();
int unlockFallenSwapSemaphore();

// Base semaphores.
extern int lockFallenBaseSemaphore();
extern int softLockFallenBaseSemaphore(
    int maxSoftTries, int* tryCount);
extern int unlockFallenBaseSemaphore();

#ifndef __GNUC__
    #define lockFallenSemaphore() \
        lockFallenBaseSemaphore()
    #define unlockFallenSemaphore() \
        unlockFallenBaseSemaphore()
    #define softLockFallenSemaphore(maxSoftTries,tryCount) \
        softLockFallenBaseSemaphore(maxSoftTries,tryCount)
#else
    #define lockFallenSemaphore() __extension__({ \
        int retval = lockFallenBaseSemaphore(); \
        retval; })
    #define unlockFallenSemaphore() __extension__({ \
        int retval = unlockFallenBaseSemaphore(); \
        retval; })
    #define softLockFallenSemaphore( \
        maxSoftTries,tryCount) __extension__({ \
        int retval = softLockFallenBaseSemaphore( \
            maxSoftTries,tryCount); \
        retval; })
#endif

//
extern void initFallenModule();

void* execFallenThread();
void updateAllFallenOnThread();

extern void respondToSurfacesSettingsChanges();
extern void boundMaxDesktopFallenDepth();
extern void setMaxDesktopFallenDepth();

extern int canFallenConsumeStormItem(FallenItem*);
extern int isFallenOnVisibleWorkspace(FallenItem*);

extern void updateFallenWithWind(FallenItem*, int w, int h);
extern void updateFallenPartial(FallenItem*, int x, int w);
extern void updateFallenAtBottom();

extern void generateFallenStormItems(FallenItem*,
    int x, int w, float vy);

// Desh method helpers.
void CreateDesh(FallenItem*);
int do_change_deshes();
int do_adjust_deshes();

//
void createFallenDisplayArea(FallenItem*);
extern void cairoDrawAllFallenItems(cairo_t*);
void eraseFallenAtPixel(FallenItem*, int x);

extern void eraseFallenOnDisplay(FallenItem*, int x, int w);
extern void freeFallenItemMemory(FallenItem*);

// FallenItem Stack Helpers.
extern void initFallenListWithDesktop();

extern void pushFallenItem(FallenItem**, WinInfo*,
    int x, int y, int w, int h);
void popFallenItem(FallenItem**);

extern FallenItem* findFallenItemByWindow(FallenItem*, Window);
extern void drawFallenItem(FallenItem*);

void swapFallenListItemSurfaces();

extern void eraseFallenListItem(Window);
extern int removeFallenListItem(FallenItem**, Window);

void removeAllFallenWindows();
void removeFallenFromWindow(Window);

extern void updateFallenRegionsWithLock(void);
extern void updateFallenRegions(void);

// Debug support.
extern void logAllFallenDisplayAreas(FallenItem*);
