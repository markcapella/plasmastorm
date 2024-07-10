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

#include "transwindow.h"
#include "windows.h"

/** *********************************************************************
 ** creates transparent window using gtk3/cairo.
 **
 ** inputStormWindow: (input)  GtkWidget to create.
 ** sticky:      (input)  Visible on all workspaces or not.
 ** below:       (input)  2: above all other windows.
 **                       1: below all other windows.
 **                       0: no action.
 ** dock:        (input)  Make it a 'dock' window: no decoration and
 **                       not interfering with app.
 **
 ** outputStormWindow:  (output) GdkWindow created
 ** x11_window:         (output) Window X11 created: (output)
 **/
int createTransparentWindow(Display* display,
    GtkWidget* inputStormWindow, int sticky, int below, int dock,
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

    // NOTE: with decorations set to TRUE the window is not
    // click-through in Gnome. So: dock = 1 is good for Gnome, or call
    // gtk_window_set_decorated(w, FALSE) before this function.
    gtk_window_set_decorated(GTK_WINDOW(inputStormWindow), FALSE);

    gtk_window_set_accept_focus(GTK_WINDOW(inputStormWindow), FALSE);

    // 'below' and 'sticky' are taken care of in gtk_main loop.
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
        return FALSE;
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

    // "So that apps like this will ignore this window."
    // TODO: No longer required as a dock? 7/8/2024 mjc
    GdkWindow* gdkStormWindow = gtk_widget_get_window(
        GTK_WIDGET(inputStormWindow));
    if (dock) {
        gdk_window_set_type_hint(gdkStormWindow,
            GDK_WINDOW_TYPE_HINT_DOCK);
    }

    gdk_window_show(gdkStormWindow);

    // Populate method result fields.
    if (x11_window) {
        *x11_window = gdk_x11_window_get_xid(gdkStormWindow);
        XResizeWindow(display, *x11_window, attr.width, attr.height);
        XFlush(display);
    }

    if (outputStormWindow) {
        *outputStormWindow = gdkStormWindow;
    }

    *wantx = 0;
    *wanty = 0;

    // TODO: Seems sometimes to be necessary with nvidia.
    // Lotsa code fix during thread inititaliztion by MJC may
    // have fixed this. Remove and test.

    // usleep(200000);
    // gtk_widget_hide(inputStormWindow);
    // gtk_widget_show_all(inputStormWindow);
    // gtk_window_move(GTK_WINDOW(inputStormWindow), 0, 0);

    // No longer slammed on the mainloop. Call once and forget.
    setStormWindowAttributes(inputStormWindow);

    g_object_steal_data(G_OBJECT(inputStormWindow), "trans_done");
    return TRUE;
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
