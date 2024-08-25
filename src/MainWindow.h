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


extern GtkWidget* getMainWindow();

extern void createMainWindow();
void init_pixmaps();

extern int isGtkVersionValid();
extern char *ui_gtk_version();
extern char *ui_gtk_required();

extern void setLanguageEnvironmentVar();
extern void respondToLanguageSettingsChanges();

extern void addBusyStyleClass();
extern void removeBusyStyleClass();

void addSliderNotAvailStyleClass();
void removeSliderNotAvailStyleClass();

void init_buttons();
extern void set_buttons();
void connectAllButtonSignals();

void setMainWindowSticky(bool);
void ui_gray_ww(const int m);
void ui_gray_below(const int m);

void onSelectedStormShapeBox(GtkComboBoxText*,
    gpointer data);
void onSelectedLanguageBox(GtkComboBoxText *combo,
    gpointer data);

void setWidgetValuesFromPrefs();

void applyUICSSTheme();
void applyCSSToWindow(GtkWidget*, GtkCssProvider*);

void setLabelText(GtkLabel*, const gchar*);

extern void logAllWindowsStackedTopToBottom();

gboolean handleMainWindowStateEvents(GtkWidget*,
    GdkEventWindowState*, gpointer);

bool startQPickerDialog(char* callerTag,
    char* colorAsString);

int getQPickerRed();
int getQPickerGreen();
int getQPickerBlue();
