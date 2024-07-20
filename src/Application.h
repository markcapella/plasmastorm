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

/***********************************************************
 * Externally provided to this Module.
 */

// Windows:
void setAppBelowAllWindows();
void setAppAboveAllWindows();

Window getActiveX11Window();
Window getActiveAppWindow();

void onCursorChange(XEvent*);
void onAppWindowChange(Window);

void onWindowCreated(XEvent*);
void onWindowReparent(XEvent*);
void onWindowChanged(XEvent*);

void onWindowMapped(XEvent*);
void onWindowFocused(XEvent*);
void onWindowBlurred(XEvent*);
void onWindowUnmapped(XEvent*);

void onWindowDestroyed(XEvent*);

bool isWindowBeingDragged();


/***********************************************************
 * Module Method stubs.
 */
void mybindtestdomain();

int drawTransparentWindow(gpointer);
void setTransparentWindowAbove(GtkWindow* window);

void setAppAboveOrBelowAllWindows();
void addWindowDrawMethodToMainloop();

void RestartDisplay();

void appShutdownHook(int);

void handleX11CairoDisplay();
int handlePendingX11Events();
int handleX11ErrorEvent(Display*, XErrorEvent*);

int drawCairoWindow(void*);
void drawCairoWindowInternal(cairo_t*);

int doAllUISettingsUpdates();
void respondToAdvancedSettingsChanges();

void HandleCpuFactor();

void respondToWorkspaceSettingsChange();
void setDesktopSession();

int handleDisplayReconfigurationChange();

int updateWindowsList();

void uninitQPickerDialog();
