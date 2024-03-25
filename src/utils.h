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

#define SOMENUMBER 42
#define PRIORITY_DEFAULT G_PRIORITY_LOW
#define PRIORITY_HIGH G_PRIORITY_DEFAULT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Intrinsic.h>
#include <gtk/gtk.h>
#include <math.h>

#ifdef HAVE_EXECINFO_H
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#define TRACEBACK_AVAILALBLE
#endif
#endif

#include "xdo.h"

extern guint addMethodToMainloop(gint prio,
    float time, GSourceFunc func);
extern guint addMethodWithArgToMainloop(gint prio,
    float time, GSourceFunc func, gpointer datap);

extern void remove_from_mainloop(guint* tag);

extern void clearStormWindow();

extern FILE *HomeOpen(const char* file, const char* mode, char** path);

extern float getWindDirection(float x);
extern float sq2(float x, float y);
extern float sq3(float x, float y, float z);
extern int randint(int m);

extern Pixel IAllocNamedColor(const char* colorName, Pixel dfltPix);
extern Pixel AllocNamedColor(const char* colorName, Pixel dfltPix);
extern void rgba2color(GdkRGBA* c, char** s);

extern void my_cairo_paint_with_alpha(cairo_t* cr, double alpha);

extern void sanelyCheckAndClearDisplayArea(Display* display,
    Window win, int x, int y, int w, int h, int exposures);

extern int hasAppScaleChangedFrom(int* prev);
extern int ValidColor(const char* color);
extern ssize_t mywrite(int fd, const void* buf, size_t count);
extern int IsReadableFile(char* path);
extern float gaussf(float x, float mu, float sigma);
extern void traceback();
extern char* getLanguageFromEnvStrings();
extern Window largest_window_with_name(xdo_t* myxdo, const char* name);
extern void fill_xdo_search(xdo_search_t* search);

extern int is_little_endian(void);
extern void logAppVersion(void);

extern void randomuniqarray(double* a, int n,
    double d, unsigned short* seed);

extern GdkRGBA getRGBFromString(char*);
