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

#include <X11/Intrinsic.h>
#include <gtk/gtk.h>
#include "plasmastorm.h"

void createStormWindow();

bool createTransparentWindow(Display* display,
    GtkWidget* inputStormWindow, int sticky, int below,
    GdkWindow** outputStormWindow, Window* x11_window,
    int* wantx, int* wanty);

int setStormWindowAttributes(GtkWidget* widget);
void setStormWindowScale();
void setStormWindowSticky(bool);

void setTransparentWindowBelow(GtkWindow* window);
void setTransparentWindowAbove(GtkWindow* window);

void logAllWindowsStackedTopToBottom();

gboolean handleTransparentWindowDrawEvents(GtkWidget*,
    cairo_t*, gpointer);
