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

// X11 Libs.
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Gtk Libs.
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
        #include "config.h"
#endif
#include "xdo.h"


/***********************************************************
 * Timer consts.
 */
#define DO_LOAD_MONITOR_EVENT_TIME 0.1
#define DO_DISPLAY_RECONFIGURATION_EVENT_TIME 0.5
#define DO_HANDLE_X11_EVENT_TIME 0.1
#define DO_UI_SETTINGS_UPDATES_EVENT_TIME 0.25
#define DO_FALLEN_THREAD_EVENT_TIME 0.2
#define DO_CAIRO_DRAW_EVENT_TIME (0.04 * mGlobal.cpufactor)
#define DO_STORMITEM_UPDATE_EVENT_TIME (0.02 * mGlobal.cpufactor)
#define DO_BLOWOFF_EVENT_TIME 0.50
#define DO_LONG_WIND_EVENT_TIME 1.00
#define DO_SHORT_WIND_EVENT_TIME 0.10
#define DO_STALL_CREATE_STORMITEM_EVENT_TIME 0.2
#define DO_CREATE_STORMITEM_EVENT 0.1

#define time_change_bottom 300.0
#define time_adjust_bottom (time_change_bottom / 20)
#define time_change_attr 60.0
#define time_show_range_etc 0.50
#define time_testing 2.10
#define time_clean 1.00
#define time_fuse 1.00
#define time_sendevent 0.5
#define time_ustar 2.00
#define time_wupdate 0.02
#define time_sfallen 2.30


/***********************************************************
 * App consts.
 */
#define XPM_TYPE const char

#ifdef NO_USE_BITS
        #define BITS(n)
#else
        #define BITS(n) :n
#endif


/***********************************************************
 * StormItem object.
 */
typedef struct _StormItem {
        unsigned int shapeType;
        GdkRGBA color;

        bool cyclic;
        bool isFrozen;
        bool isVisible;

        bool fluff;
        float flufftimer;
        float flufftime;

        // Position values.
        float xRealPosition;
        int xIntPosition;

        float yRealPosition;
        int yIntPosition;

        // Physics.
        float massValue;
        float windSensitivity;

        float initialYVelocity;

        float xVelocity;
        float yVelocity;
} StormItem;


/***********************************************************
 * StormItemSurface object.
 */
typedef struct _StormItemSurface {
        cairo_surface_t* surface;

        unsigned int width BITS(16);
        unsigned int height BITS(16);
} StormItemSurface;


/***********************************************************
 * Star objects.
 */
typedef struct _StarMap {
        unsigned char* starBits;

        Pixmap pixmap;

        int width;
        int height;
} StarMap;

typedef struct _StarCoordinate {
        int x;
        int y;

        int color;
} StarCoordinate;

/***********************************************************
 * WinInfo object.
 */
typedef struct _WinInfo {

        Window window;     // Window.
        long ws;           // workspace

        int x, y;          // x,y coordinates
        int xa, ya;        // x,y coordinates absolute
        unsigned int w, h; // width, height

        unsigned int sticky BITS(1); // is visible on all workspaces
        unsigned int dock BITS(1);   // is a "dock" (panel)
        unsigned int hidden BITS(1); // is hidden / iconized
} WinInfo;


/***********************************************************
 * Fallen object.
 */
typedef struct _FallenItem {
        WinInfo winInfo;          // winInfo None == bottom.
        struct _FallenItem* next; // pointer to next item.

        cairo_surface_t* surface;
        cairo_surface_t* surface1;

        short int x, y;           // X, Y array.
        short int w, h;           // W, H array.

        int prevx, prevy;         // x, y of last draw.
        int prevw, prevh;         // w, h of last draw.

        GdkRGBA* columnColor;     // Color array.
        short int* fallenHeight;    // actual heights.
        short int* maxFallenHeight; // desired heights.
} FallenItem;


/***********************************************************
 * Global helper objects.
 */
#define MAXIMUM_GLOBAL_WORKSPACES 100

extern struct _mGlobal {
        bool isStormWindowTransparent;
        char* DesktopSession;

        bool isCompizCompositor;
        int isWaylandDisplay;
        int windowsWereDraggedOrMapped;
        bool languageChangeRestart;

        // mGlobal.Rootwindow or mGlobal.StormWindow.
        Bool hasDestopWindow;

        // mGlobal.Rootwindow.
        Window Rootwindow;
        int Xroot;
        int Yroot;
        unsigned int Wroot;
        unsigned int Hroot;

        // mGlobal.StormWindow.
        Window StormWindow;
        int StormWindowX;
        int StormWindowY;
        int StormWindowWidth;
        int StormWindowHeight;

        char* Language;

        unsigned int MaxStormItemHeight;
        unsigned int MaxStormItemWidth;
        int StormItemCount;
        int FluffedStormItemCount;
        int RemoveFluff;

        int Wind; // 0 = None, 1 = blow stormItem, 2 = blowSanta.
        int Direction; // 0 = no, 1 = LTR, 2 = RTL.
        float NewWind;
        float WindMax;
        float windWhirlValue;
        double windWhirlTimer;
        double windWhirlTimerStart;

        int WindowOffsetWindowTops;
        int WindowOffsetX;
        int MaxDesktopFallenDepth;
        FallenItem* FallenFirst;

        double cpufactor;
        float WindowScale;

        int NVisWorkSpaces;
        long CWorkSpace;
        long ChosenWorkSpace;
        long workspaceArray[MAXIMUM_GLOBAL_WORKSPACES];

        Display* display;
        xdo_t* xdo;
        int ComboStormShape;


} mGlobal;
