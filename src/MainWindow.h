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

// Required GTK version.
#define GTK_MAJOR 3
#define GTK_MINOR 20
#define GTK_MICRO 0

extern void respondToLanguageSettingsChanges();
extern void initializeMainWindow();

extern void ui_set_sticky(int x);

extern void addBusyStyleClass();
extern void removeBusyStyleClass();

void addSliderNotAvailStyleClass();
void removeSliderNotAvailStyleClass();

extern void ui_gray_ww(const int m);
extern void ui_gray_below(const int m);
extern int isGtkVersionValid();
extern char *ui_gtk_version();
extern char *ui_gtk_required();

extern void set_buttons();
extern void setLanguageEnvironmentVar();

void init_buttons();
void connectAllButtonSignals();
void init_pixmaps();

void onSelectedStormShapeBox(GtkComboBoxText*,
    gpointer data);
void onSelectedLanguageBox(GtkComboBoxText *combo,
    gpointer data);
