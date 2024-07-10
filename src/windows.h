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

#include <stdbool.h>

#include <X11/Intrinsic.h>
#include <gtk/gtk.h>

#include "plasmastorm.h"

extern void addWindowsModuleToMainloop(void);

extern int WorkspaceActive(void);

extern void initDisplayDimensions();
extern void updateDisplayDimensions(void);

extern void SetBackground(void);

extern bool isAreaClippedByWindow(int, int,
    unsigned, unsigned);

void getWinInfoList();

/***********************************************************
 * Externally provided to this Module.
 */
bool is_NET_WM_STATE_Hidden(Window window);
bool is_WM_STATE_Hidden(Window window);

void uninitQPickerDialog();
void endApplication(void);


/***********************************************************
 * Module Method stubs.
 */
//**
// Workspace.
void udpateWorkspaceInfo();
int do_sendevent();
extern int updateWindowsList();

//**
// Active & Focused X11 Window Helper methods.
extern Window getActiveX11Window();
Window getFocusedX11Window();
int getFocusedX11XPos();
int getFocusedX11YPos();

void clearAllActiveAppFields();

extern Window getActiveAppWindow();
void setActiveAppWindow(Window);
Window getParentOfActiveAppWindow();

int getActiveAppXPos();
void setActiveAppXPos(int);

int getActiveAppYPos();
void setActiveAppYPos(int);

//**
// Windows life-cycle methods.
extern void onCursorChange(XEvent*);
extern void onAppWindowChange(Window);

extern void onWindowCreated(XEvent*);
extern void onWindowReparent(XEvent*);
extern void onWindowChanged(XEvent*);

extern void onWindowMapped(XEvent*);
extern void onWindowFocused(XEvent*);
extern void onWindowBlurred(XEvent*);
extern void onWindowUnmapped(XEvent*);

extern void onWindowDestroyed(XEvent*);

//**
// Windows life-cycle helper methods.
bool isMouseClickedAndHeldInWindow();

void clearAllDragFields();

extern bool isWindowBeingDragged();
void setIsWindowBeingDragged(bool);

Window getWindowBeingDragged();
void setWindowBeingDragged(Window);

Window getActiveAppDragWindowCandidate();
void setActiveAppDragWindowCandidate(Window);

Window getDragWindowOf(Window);

//**
// Debug methods.
void logCurrentTimestamp();
void logWindowAndAllParents(Window window);

WinInfo* findWinInfoByWindowId(Window);
