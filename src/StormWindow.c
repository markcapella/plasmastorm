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
#include <stdbool.h>

#include <X11/Intrinsic.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "Application.h"
#include "ColorCodes.h"
#include "Prefs.h"
#include "StormWindow.h"
#include "Windows.h"
#include "utils.h"


/** *********************************************************************
 ** Module globals and consts.
 **/

static int wantx = 0;
static int wanty = 0;
static int mIsSticky = 0;

/** *********************************************************************
 ** This method ...
 **/
void createStormWindow() {
    mGlobal.Rootwindow =
        DefaultRootWindow(mGlobal.display);

    mGlobal.isStormWindowTransparent = false;
    mGlobal.isCairoAvailable = false;

    mGlobal.hasDestopWindow = true;

    // Try to create a transparent clickthrough window.
    GtkWidget* stormWindowWidget = gtk_message_dialog_new(NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER,
        GTK_BUTTONS_NONE, "unused");

    gtk_widget_set_can_focus(stormWindowWidget, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(stormWindowWidget), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(stormWindowWidget),
        GDK_WINDOW_TYPE_HINT_POPUP_MENU);

    // Create transparent StormWindow widget.
    Window stormX11Window;
    GdkWindow* stormGdkWindow;

    if (createTransparentWindow(mGlobal.display, stormWindowWidget,
        Flags.AllWorkspaces, true, &stormGdkWindow,
        &stormX11Window, &wantx, &wanty)) {

        mGlobal.gtkStormWindowWidget = stormWindowWidget;
        mGlobal.isStormWindowTransparent = true;
        mGlobal.StormWindow = stormX11Window;

        gtk_window_set_icon_from_file(GTK_WINDOW(mGlobal.gtkStormWindowWidget),
            "/usr/share/icons/hicolor/48x48/apps/"
            "plasmastorm.png", NULL);

        GtkWidget* drawing_area = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(mGlobal.gtkStormWindowWidget),
            drawing_area);

        g_signal_connect(mGlobal.gtkStormWindowWidget, "draw",
            G_CALLBACK(handleTransparentWindowDrawEvents), NULL);

        printf("%sTransparent StormWindow created.%s\n\n",
            COLOR_NORMAL, COLOR_NORMAL);

    } else {
        mGlobal.isCairoAvailable = true;

        // Find global StormWindow based on desktop.
        Window desktopAsStormWindow = None;
        if (strncmp(getDesktopSession(), "LXDE", 4) == 0) {
            desktopAsStormWindow = largest_window_with_name(
                mGlobal.xdo, "^pcmanfm$");
        }
        if (!desktopAsStormWindow) {
            desktopAsStormWindow = largest_window_with_name(
            mGlobal.xdo, "^Desktop$");
        }
        if (!desktopAsStormWindow) {
            desktopAsStormWindow = mGlobal.Rootwindow;
        }
        mGlobal.StormWindow = desktopAsStormWindow;

        printf("\n%splasmastorm:application: createStormWindow() "
            "Transparent click-thru StormWindow not available.%s\n\n",
            COLOR_YELLOW, COLOR_NORMAL);
    }

    // Start initial window screen position.
    if (mGlobal.isCairoAvailable) {
        handleX11CairoDisplay();
        mGlobal.cairoWindowGuid = addMethodWithArgToMainloop(PRIORITY_HIGH,
            DO_CAIRO_DRAW_EVENT_TIME, drawCairoWindow, mGlobal.cairoWindow);
        mGlobal.windowOffsetX = 0;
        mGlobal.windowOffsetY = 0;
    } else {
        mGlobal.windowOffsetX = wantx;
        mGlobal.windowOffsetY = wanty;
    }

    mGlobal.StormWindowX = wantx;
    mGlobal.StormWindowY = wanty;

    if (!mGlobal.isCairoAvailable) {
        xdo_move_window(mGlobal.xdo, mGlobal.StormWindow,
            mGlobal.StormWindowX, mGlobal.StormWindowY);
    }
    xdo_wait_for_window_map_state(mGlobal.xdo,
        mGlobal.StormWindow, IsViewable);

    initDisplayDimensions();

    setStormWindowScale();

    fflush(stdout);
}

/** *********************************************************************
 ** creates transparent window using gtk3/cairo.
 **
 ** inputStormWindow: (input)  GtkWidget to create.
 ** sticky:      (input)  Visible on all workspaces or not.
 ** below:       (input)  2: above all other windows.
 **                       1: below all other windows.
 **                       0: no action.
 **
 ** outputStormWindow:  (output) GdkWindow created
 ** x11_window:         (output) Window X11 created: (output)
 **/
bool createTransparentWindow(Display* display,
    GtkWidget* inputStormWindow, int sticky, int below,
    GdkWindow** outputStormWindow, Window* x11_window,
    int* wantx, int* wanty) {

    // Guard the outputs.
    if (outputStormWindow) {
        *outputStormWindow = NULL;
    }
    if (x11_window) {
        *x11_window = None;
    }

    // Implement window.
    gtk_widget_set_app_paintable(inputStormWindow, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(inputStormWindow), FALSE);
    gtk_window_set_accept_focus(GTK_WINDOW(inputStormWindow), FALSE);

    g_signal_connect(inputStormWindow, "draw",
        G_CALLBACK(setStormWindowAttributes), NULL);

    // Remove our things from inputStormWindow:
    g_object_steal_data(G_OBJECT(inputStormWindow), "trans_sticky");
    g_object_steal_data(G_OBJECT(inputStormWindow), "trans_nobelow");
    g_object_steal_data(G_OBJECT(inputStormWindow), "trans_below");
    g_object_steal_data(G_OBJECT(inputStormWindow), "trans_done");

    // Reset our things.  :-/
    static char unusedResult;
    if (sticky) {
        g_object_set_data(G_OBJECT(inputStormWindow),
            "trans_sticky", &unusedResult);
    }
    switch (below) {
        case 0:
            g_object_set_data(G_OBJECT(inputStormWindow),
                "trans_nobelow", &unusedResult);
            break;
        case 1:
            g_object_set_data(G_OBJECT(inputStormWindow),
                "trans_below", &unusedResult);
            break;
    }

    // To check if the display supports alpha channels, get the visual.
    GdkScreen* screen = gtk_widget_get_screen(inputStormWindow);
    if (!gdk_screen_is_composited(screen)) {
        gtk_window_close(GTK_WINDOW(inputStormWindow));
        return false;
    }

    // Ensure the widget (the window, actually) can take RGBA
    gtk_widget_set_visual(inputStormWindow,
        gdk_screen_get_rgba_visual(screen));

    // set full screen if so desired:
    XWindowAttributes attr;
    XGetWindowAttributes(display, DefaultRootWindow(display), &attr);
    gtk_widget_set_size_request(GTK_WIDGET(inputStormWindow),
        attr.width, attr.height);

    gtk_widget_show_all(inputStormWindow);

    // Set return values.
    if (outputStormWindow) {
        *outputStormWindow = gtk_widget_get_window(
            GTK_WIDGET(inputStormWindow));
    }
    if (x11_window) {
        *x11_window = gdk_x11_window_get_xid(
            gtk_widget_get_window(GTK_WIDGET(inputStormWindow)));
        XResizeWindow(display, *x11_window, attr.width, attr.height);
        XFlush(display);
    }
    *wantx = 0;
    *wanty = 0;

    // No longer slammed on the mainloop.
    // Call once and forget.
    setStormWindowAttributes(inputStormWindow);

    g_object_steal_data(G_OBJECT(inputStormWindow),
        "trans_done");
    return true;
}

/** *********************************************************************
 **
 ** for some reason, in some environments the 'below' and 'stick'
 ** properties disappear. It works again, if we express our
 ** wishes after starting gtk_main and the best place is in the
 ** draw event.
 ** 
 ** We want to reset the settings at least once to be sure.
 ** Things like sticky and below should be stored in the
 ** widget beforehand.
 **
 ** TODO:
 ** Lotsa code fix during thread inititaliztion by MJC may
 ** have fixed this. Tinker and test.
 **/
int setStormWindowAttributes(GtkWidget* stormWindow) {
    enum { maxResetCount = 1, numResetAttributes }; 
    static char maxResetAttributes[numResetAttributes];

    char* resultPointer = (char*)
        g_object_get_data(G_OBJECT(stormWindow), "trans_done");
    if (!resultPointer) {
        resultPointer = &maxResetAttributes[0];
    }
    if (resultPointer - &maxResetAttributes[0] >=
            maxResetCount) {
        return false;
    }
    g_object_set_data(G_OBJECT(stormWindow),
        "trans_done", ++resultPointer);

    // Create a new region, then combine it with the
    // storm window, then destroy it.
    // TODO: Why? ... "Does not work as expected."

    GdkWindow* gdk_window1 = gtk_widget_get_window(stormWindow);
    //const int Usepassthru = 0;
    //if (Usepassthru) {
    //    gdk_window_set_pass_through(gdk_window1, true);
    //} else {
        cairo_region_t* cairo_region1 = cairo_region_create();
        gdk_window_input_shape_combine_region(gdk_window1,
            cairo_region1, 0, 0);
        cairo_region_destroy(cairo_region1);
    //}

    if (!g_object_get_data(G_OBJECT(stormWindow), "trans_nobelow")) {
        if (g_object_get_data(G_OBJECT(stormWindow), "trans_below")) {
            setTransparentWindowBelow(GTK_WINDOW(stormWindow));
        } else {
            setTransparentWindowAbove(GTK_WINDOW(stormWindow));
        }
    }

    // Set the Trans Window Sticky Flag.
    if (g_object_get_data(G_OBJECT(stormWindow), "trans_sticky")) {
        gtk_window_stick(GTK_WINDOW(stormWindow));
    } else {
        gtk_window_unstick(GTK_WINDOW(stormWindow));
    }

    return false;
}

/** *********************************************************************
 ** 
 **/
void setTransparentWindowBelow(__attribute__((unused)) GtkWindow* window) {
    gtk_window_set_keep_above(GTK_WINDOW(window), false);
    gtk_window_set_keep_below(GTK_WINDOW(window), true);
}

/** *********************************************************************
 ** 
 **/
void setTransparentWindowAbove(__attribute__((unused)) GtkWindow* window) {
    gtk_window_set_keep_below(GTK_WINDOW(window), false);
    gtk_window_set_keep_above(GTK_WINDOW(window), true);
}

/** *********************************************************************
 ** Set the Transparent Window Sticky Flag.
 **/
void setStormWindowSticky(bool isSticky) {
    mIsSticky = isSticky;
    if (mIsSticky) {
        gtk_window_stick(GTK_WINDOW(
            mGlobal.gtkStormWindowWidget));
    } else {
        gtk_window_unstick(GTK_WINDOW(
            mGlobal.gtkStormWindowWidget));
    }
}

/** *********************************************************************
 ** This method id the draw callback.
 **/
gboolean handleTransparentWindowDrawEvents(
    __attribute__((unused)) GtkWidget *widget, cairo_t *cr,
    __attribute__((unused)) gpointer user_data) {

    drawCairoWindowInternal(cr);
    return FALSE;
}

/** *********************************************************************
 ** This method ... TODO: Huh?
    // assuming a standard screen of 1024x576, we suggest to use the scalefactor
    // WindowScale
 **/
void setStormWindowScale() {
    float x = mGlobal.StormWindowWidth / 1000.0;
    float y = mGlobal.StormWindowHeight / 576.0;

    if (x < y) {
        mGlobal.WindowScale = x;
    } else {
        mGlobal.WindowScale = y;
    }
}
