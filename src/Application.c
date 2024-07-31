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

// Std C Lib headers.
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// X11 headers.
#include <X11/Intrinsic.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xfixes.h>
#include <X11/cursorfont.h>

// Gnome library headers.
#include <gtk/gtk.h>

// Spline library.
#include <gsl/gsl_version.h>

// Graphics library.
#include <cairo-xlib.h>

// plasmastorm headers.
#include "plasmastorm.h"

#include "Application.h"
#include "Blowoff.h"
#include "ClockHelper.h"
#include "ColorCodes.h"
#include "Fallen.h"
#include "loadmeasure.h"
#include "mainstub.h"
#include "MainWindow.h"
#include "mygettext.h"
#include "Prefs.h"
#include "safeMalloc.h"
#include "Stars.h"
#include "Storm.h"
#include "StormWindow.h"
#include "utils.h"
#include "versionHelper.h"
#include "Wind.h"
#include "Windows.h"
#include "x11WindowHelper.h"


/** *********************************************************************
 ** Module globals and consts.
 **/

// mGlobal object base.
struct _mGlobal mGlobal;

static guint mTransparentWindowGUID = 0;

Bool mMainWindowNeedsReconfiguration = true;

static int mPrevStormWindowWidth = 0;
static int mPrevStormWindowHeight = 0;

const int mX11MaxErrorCount = 500;
static int mX11ErrorCount = 0;

int mX11LastErrorCode = 0;


/** *********************************************************************
 ** Application start method. 
 **/
int startApplication(int argc, char *argv[]) {
    // Log info, version checks.
    logAppVersion();

    printf("Available languages are:\n%s.\n\n",
        LANGUAGES);

    printf("GTK version: %s\n", ui_gtk_version());
    #ifdef GSL_VERSION
        fprintf(stdout, "GSL version: %s\n\n", GSL_VERSION);
    #else
        fprintf(stdout, "GSL version: UNKNOWN\n\n");
    #endif

    if (!isGtkVersionValid()) {
        printf("%splasmastorm: needs gtk version >= %s, "
            "found version %s.%s\n", COLOR_RED,
            ui_gtk_required(), ui_gtk_version(), COLOR_NORMAL);
        return 0;
    }

    printf("%splasmastorm: Desktop %s detected.%s\n\n",
        COLOR_BLUE, getDesktopSession() ? getDesktopSession() :
        "was not", COLOR_NORMAL);

    // Log Wayland info.
    const bool isWaylandPresent = getenv("WAYLAND_DISPLAY") &&
        getenv("WAYLAND_DISPLAY") [0];
    if (isWaylandPresent) {
        printf("%splasmastorm: Wayland display was "
            "detected - FATAL.%s\n\n", COLOR_RED, COLOR_NORMAL);
        exit(1);
    }

    // Before starting GTK, ensure x11 backend is used.
    setenv("GDK_BACKEND", "x11", 1);

    // Set app shutdown handlers.
    signal(SIGINT, appShutdownHook);
    signal(SIGTERM, appShutdownHook);
    signal(SIGHUP, appShutdownHook);

    // Seed random generator.
    srand48((int) (fmod(wallcl() * 1.0e6, 1.0e8)));

    // Clear space for app mGlobal struct.
    memset(&mGlobal, 0, sizeof(mGlobal));

    mGlobal.windowsWereDraggedOrMapped = 0;
    mGlobal.languageChangeRestart = false;

    mGlobal.StormWindow = None;
    mGlobal.MaxStormItemHeight = 0;
    mGlobal.MaxStormItemWidth = 0;
    mGlobal.StormItemCount = 0;
    mGlobal.FluffedStormItemCount = 0;
    mGlobal.RemoveFluff = 0;

    mGlobal.Wind = 0;
    mGlobal.Direction = 0;
    mGlobal.WindMax = 500.0;
    mGlobal.NewWind = 100.0;

    mGlobal.windowOffsetY = 0;
    mGlobal.windowOffsetX = 0;
    mGlobal.MaxDesktopFallenDepth = 0;
    mGlobal.FallenFirst = NULL;

    mGlobal.cpufactor = 1.0;
    mGlobal.WindowScale = 1.0;

    mGlobal.visibleWorkspaceCount = 1;
    mGlobal.currentWS = 0;
    mGlobal.chosenWorkSpace = 0;
    mGlobal.workspaceArray[0] = 0;


    mGlobal.display = XOpenDisplay("");
    if (mGlobal.display == NULL) {
        printf("plasmastorm: cannot connect "
            "to X server - FATAL.\n"),
        exit(1);
    }

    mGlobal.xdo = xdo_new_with_opened_display(
        mGlobal.display, NULL, 0);
    if (mGlobal.xdo == NULL) {
        printf("plasmastorm: needs xdo and it reports "
            "no displays - FATAL.\n");
        exit(1);
    }

    // Init flags module.
    initPrefsModule(argc, argv);

    // Handle langauge localizations.
    // Language locale string bindings.
    setLanguageEnvironmentVar();
    mGlobal.Language = getLanguageFromEnvStrings();
    mybindtestdomain();

    initFallenSemaphores();

    // Init it!
    gtk_init(&argc, &argv);
    writePrefstoLocalStorage();

    mGlobal.xdo->debug = 0;
    XSynchronize(mGlobal.display, 0);
    XSetErrorHandler(handleX11ErrorEvent);

    // Start Main GUI Window.
    setStormShapeColor(getRGBFromString(Flags.StormItemColor1));

    createStormWindow();

    mPrevStormWindowWidth = mGlobal.StormWindowWidth;
    mPrevStormWindowHeight = mGlobal.StormWindowHeight;

    // Init all Global Flags.
    OldFlags.shutdownRequested = Flags.shutdownRequested;
    OldFlags.mHaveFlagsChanged = Flags.mHaveFlagsChanged;

    OldFlags.Language = strdup(Flags.Language);

    OldFlags.ShowStormItems = Flags.ShowStormItems;
    OldFlags.StormItemColor1 = strdup(Flags.StormItemColor1);
    OldFlags.StormItemColor2 = strdup(Flags.StormItemColor2);
    OldFlags.ShapeSizeFactor = Flags.ShapeSizeFactor;
    OldFlags.StormItemSpeedFactor = Flags.StormItemSpeedFactor;
    OldFlags.StormItemCountMax = Flags.StormItemCountMax;
    OldFlags.StormSaturationFactor = Flags.StormSaturationFactor;

    OldFlags.ShowStars = Flags.ShowStars;
    OldFlags.MaxStarCount = Flags.MaxStarCount;

    OldFlags.ShowWind = Flags.ShowWind;
    OldFlags.WhirlFactor = Flags.WhirlFactor;
    OldFlags.WhirlTimer = Flags.WhirlTimer;
    OldFlags.ShowBlowoff = Flags.ShowBlowoff;
    OldFlags.BlowOffFactor = Flags.BlowOffFactor;

    OldFlags.KeepFallenOnWindows = Flags.KeepFallenOnWindows;
    OldFlags.MaxWindowFallenDepth = Flags.MaxWindowFallenDepth;
    OldFlags.WindowFallenTopOffset = Flags.WindowFallenTopOffset;
    OldFlags.KeepFallenOnDesktop = Flags.KeepFallenOnDesktop;
    OldFlags.MaxDesktopFallenDepth = Flags.MaxDesktopFallenDepth;
    OldFlags.DesktopFallenTopOffset = Flags.DesktopFallenTopOffset;

    OldFlags.CpuLoad = Flags.CpuLoad;
    OldFlags.Transparency = Flags.Transparency;
    OldFlags.Scale = Flags.Scale;
    OldFlags.AllWorkspaces = Flags.AllWorkspaces;
    OldFlags.ComboStormShape = Flags.ComboStormShape;

    // Init the X11Threads.
    XInitThreads();

    // Request all interesting X11 events.
    const Window eventWindow = (mGlobal.hasDestopWindow) ?
        mGlobal.Rootwindow : mGlobal.StormWindow;

    XSelectInput(mGlobal.display, eventWindow,
        StructureNotifyMask | SubstructureNotifyMask |
        FocusChangeMask);
    XFixesSelectCursorInput(mGlobal.display, eventWindow,
        XFixesDisplayCursorNotifyMask);

    clearStormWindow();

    createMainWindown();

    // Hide us if starting minimized.
    if (Flags.mHideMenu) {
        gtk_window_iconify(GTK_WINDOW(getMainWindow()));
    }

    setMainWindowSticky(Flags.AllWorkspaces);

    Flags.shutdownRequested = false;
    updateWindowsList();

    // Init app modules.
    addWindowsModuleToMainloop();

    initWindModule();
    initBlowoffModule();
    initFallenModule();
    initStarsModule();
    initStormModule();

    addLoadMonitorToMainloop();

    addMethodToMainloop(PRIORITY_DEFAULT,
        DO_HANDLE_X11_EVENT_TIME, handlePendingX11Events);

    addMethodToMainloop(PRIORITY_DEFAULT,
        DO_DISPLAY_RECONFIGURATION_EVENT_TIME,
        handleDisplayReconfigurationChange);

    addMethodToMainloop(PRIORITY_HIGH,
        DO_UI_SETTINGS_UPDATES_EVENT_TIME,
        doAllUISettingsUpdates);

    HandleCpuFactor();
    respondToWorkspaceSettingsChange();

    // Log Storming window status.
    printf("%splasmastorm: It\'s Storming in:%s\n",
        COLOR_CYAN, COLOR_NORMAL);

    logWindow(mGlobal.StormWindow);
    fflush(stdout);

    //***************************************************
    // Bring it all up !
    //***************************************************

    printf("\n%splasmastorm: gtk_main() Starts.%s\n",
        COLOR_BLUE, COLOR_NORMAL);

    gtk_main();

    printf("\n%splasmastorm: gtk_main() Finishes.%s\n",
        COLOR_BLUE, COLOR_NORMAL);

    // Display termination messages to MessageBox or STDOUT.
     printf("%s\nThanks for using plasmastorm, you rock !%s\n",
         COLOR_GREEN, COLOR_NORMAL);

    // More terminates.
    XClearWindow(mGlobal.display, mGlobal.StormWindow);
    XFlush(mGlobal.display);

    XCloseDisplay(mGlobal.display);
    uninitQPickerDialog();

    // If Restarting due to display or language change.
    if (mGlobal.languageChangeRestart) {
        sleep(0);
        setenv("plasmastorm_RESTART", "yes", 1);
        execvp(argv[0], argv);
    }

    return 0;
}

/** *********************************************************************
 ** Set the app x11 window order based on user pref.
 **/
void setAppAboveOrBelowAllWindows() {
    setAppBelowAllWindows();
}

/** *********************************************************************
 ** This method gets the desktop session type from env vars.
 **/
char* getDesktopSession() {
    const char* DESKTOPS[] = {
        "DESKTOP_SESSION", "XDG_SESSION_DESKTOP",
        "XDG_CURRENT_DESKTOP", "GDMSESSION", NULL};

    char* desktopsession;
    for (int i = 0; DESKTOPS[i]; i++) {
        desktopsession = getenv(DESKTOPS[i]);
        if (desktopsession) {
            break;
        }
    }

    return desktopsession;
}

/** *********************************************************************
 ** Cairo specific. handleDisplayReconfigurationChange()
 **/
void handleX11CairoDisplay() {
    unsigned int width, height;
    xdo_get_window_size(mGlobal.xdo, mGlobal.StormWindow,
        &width, &height);

    printf("handleX11CairoDisplay() Double buffers unavailable "
        "or unrequested.\n");

    Visual* visual = DefaultVisual(mGlobal.display,
        DefaultScreen(mGlobal.display));
    mGlobal.cairoSurface = cairo_xlib_surface_create(mGlobal.display,
        mGlobal.StormWindow, visual, width, height);

    // Destroy & create new Cairo Window.
    if (mGlobal.cairoWindow) {
        cairo_destroy(mGlobal.cairoWindow);
    }
    mGlobal.cairoWindow = cairo_create(mGlobal.cairoSurface);
    cairo_xlib_surface_set_size(mGlobal.cairoSurface, width, height);

    mGlobal.StormWindowWidth = width;
    mGlobal.StormWindowHeight = height;
}

/** *********************************************************************
 * here we are handling the buttons in ui
 * Ok, this is a long list, and could be implemented more efficient.
 * But, doAllUISettingsUpdates is called not too frequently, so....
 * Note: if changes != 0, the settings will be written to .plasmastormrc
 **/
int doAllUISettingsUpdates() {
    if (Flags.shutdownRequested) {
        gtk_main_quit();
    }

    respondToLanguageSettingsChanges();

    respondToStormsSettingsChanges();
    updateStarsUserSettings();
    respondToWindSettingsChanges();
    respondToSurfacesSettingsChanges();
    respondToBlowoffSettingsChanges();

    respondToAdvancedSettingsChanges();

    return TRUE;
}

/** *********************************************************************
 ** TODO: Advanced Page for UI should be in its own module.
 **/
void respondToAdvancedSettingsChanges() {
    //UIDO(CpuLoad, 
    if (Flags.CpuLoad !=
            OldFlags.CpuLoad) {
        OldFlags.CpuLoad =
            Flags.CpuLoad;
        HandleCpuFactor();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(Transparency, );
    if (Flags.Transparency !=
            OldFlags.Transparency) {
        OldFlags.Transparency =
            Flags.Transparency;
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(Scale, );
    if (Flags.Scale !=
            OldFlags.Scale) {
        OldFlags.Scale =
            Flags.Scale;
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(AllWorkspaces, 
    if (Flags.AllWorkspaces !=
            OldFlags.AllWorkspaces) {
        OldFlags.AllWorkspaces =
            Flags.AllWorkspaces;
        respondToWorkspaceSettingsChange();
        Flags.mHaveFlagsChanged++;
    }

    // TODO: Advanced Settings changes might trigger restart
    // on display change so (?) we force the prefs out. This should be
    // closer to the actual codepath not catch all here :-(
    if (Flags.mHaveFlagsChanged > 0) {
        writePrefstoLocalStorage();
        set_buttons();
        Flags.mHaveFlagsChanged = 0;
    }
}

/** *********************************************************************
 ** 
 **/
void respondToWorkspaceSettingsChange() {
    if (Flags.AllWorkspaces) {
        if (mGlobal.isStormWindowTransparent) {
            setStormWindowSticky(true);
        }
    } else {
        if (mGlobal.isStormWindowTransparent) {
            setStormWindowSticky(false);
        }
        mGlobal.chosenWorkSpace = mGlobal.workspaceArray[0];
    }

    setMainWindowSticky(Flags.AllWorkspaces);
}

/** *********************************************************************
 ** This method handles xlib11 event notifications.
 ** Undergoing heavy renovation 2024 Rocks !
 **/
int handlePendingX11Events() {
    if (Flags.shutdownRequested) {
        return FALSE;
    }

    XFlush(mGlobal.display);
    while (XPending(mGlobal.display)) {
        // printf("%sApplication: handlePendingX11Events() Starts.%s\n",
        //     COLOR_YELLOW, COLOR_NORMAL);

        XEvent event;
        XNextEvent(mGlobal.display, &event);

        // Check for Active window change through loop.
        const Window activeX11Window = getActiveX11Window();
        if (getActiveAppWindow() != activeX11Window) {
            // printf("%sApplication: handlePendingX11Events() Start : "
            //     "onAppWindowChange.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
            onAppWindowChange(activeX11Window);
            // printf("%sApplication: handlePendingX11Events() Finish : "
            //     "onAppWindowChange.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
        }

        // Perform X11 event action.
        switch (event.type) {
            case CreateNotify:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowCreated.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowCreated(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowCreated.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case ReparentNotify:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowReparent.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowReparent(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowReparent.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case ConfigureNotify:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowChanged.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowChanged(&event);
                if (!isWindowBeingDragged()) {
                    mGlobal.windowsWereDraggedOrMapped++;
                    if (event.xconfigure.window == mGlobal.StormWindow) {
                        mMainWindowNeedsReconfiguration = true;
                    }
                }
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowChanged.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case MapNotify:
                // printf("%splasmastorm::Application: "
                //     "handlePendingX11Events() Start : "
                //      "onWindowMapped.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                mGlobal.windowsWereDraggedOrMapped++;
                onWindowMapped(&event);
                // printf("%splasmastorm::Application: "
                //     "handlePendingX11Events() Finish : "
                //      "onWindowMapped.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case FocusIn:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowFocused.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowFocused(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowFocused.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case FocusOut:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowBlurred.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowBlurred(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowBlurred.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case UnmapNotify:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowUnmapped.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                mGlobal.windowsWereDraggedOrMapped++;
                onWindowUnmapped(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowUnmapped.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            case DestroyNotify:
                // printf("%sApplication: handlePendingX11Events() Start : "
                //     "onWindowDestroyed.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                onWindowDestroyed(&event);
                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "onWindowDestroyed.%s\n\n", COLOR_BLUE, COLOR_NORMAL);
                break;

            default:
                // Check & perform XFixes action.
                int xfixes_event_base;
                int xfixes_error_base;

                if (XFixesQueryExtension(mGlobal.display,
                    &xfixes_event_base, &xfixes_error_base)) {
                    switch (event.type - xfixes_event_base) {
                        case XFixesCursorNotify:
                            onCursorChange(&event);
                            // printf("%sApplication: handlePendingX11Events() "
                            //     "XFixesCursorNotify Event.%s\n\n",
                            //     COLOR_BLUE, COLOR_NORMAL);
                            break;
                    }
                } else {
                    // printf("%sApplication: handlePendingX11Events() "
                    //     "UN-KNOWN XFixesQueryExtension Event.%s\n\n",
                    //     COLOR_RED, COLOR_NORMAL);
                    // break;
                }

                // printf("%sApplication: handlePendingX11Events() Finish : "
                //     "UN-KNOWN EVENT ???.%s\n\n", COLOR_RED, COLOR_NORMAL);
                break;
        }
    }

    // printf("%sApplication: handlePendingX11Events() : Finishes.%s\n\n",
    //     COLOR_YELLOW, COLOR_NORMAL);
    return TRUE;
}

/** *********************************************************************
 ** This method ...
 **/
void RestartDisplay() {
    fflush(stdout);

    initFallenListWithDesktop();

    clearStormWindow();
}

/** *********************************************************************
 ** This method logs signal event shutdowns as fyi.
 **/
void appShutdownHook(int signalNumber) {
    printf("%splasmastorm: Shutdown by Signal Handler : %i.%s\n",
        COLOR_YELLOW, signalNumber, COLOR_NORMAL);

    Flags.shutdownRequested = true;
}

/** *********************************************************************
 ** This method traps and handles X11 errors.
 **
 ** Primarily, we close the app if the system doesn't seem sane.
 **/
int handleX11ErrorEvent(Display* dpy, XErrorEvent* event) {
    // Save error & quit early if simply BadWindow.
    mX11LastErrorCode = event->error_code;
    if (mX11LastErrorCode == BadWindow) {
        return 0;
    }

    // Print the error message of the event.
    const int MAX_MESSAGE_BUFFER_LENGTH = 60;
    char msg[MAX_MESSAGE_BUFFER_LENGTH];
    XGetErrorText(dpy, event->error_code, msg, sizeof(msg));
    printf("%splasmastorm::Application handleX11ErrorEvent() %s.%s\n",
        COLOR_RED, msg, COLOR_NORMAL);

    // Halt after too many errors.
    if (mX11ErrorCount++ > mX11MaxErrorCount) {
        printf("\n%splasmastorm: Shutting down due to excessive "
            "X11 errors.%s\n", COLOR_RED, COLOR_NORMAL);
        Flags.shutdownRequested = true;
    }

    return 0;
}

/** *********************************************************************
 ** This method ...
 **/
int drawCairoWindow(void *cr) {
    drawCairoWindowInternal((cairo_t *) cr);
    return TRUE;
}

/** *********************************************************************
 ** Due to instabilities at the start of app, we are
 ** repeated a few times. This is not harmful. We do not draw
 ** anything the first few times this function is called.
 **/
void drawCairoWindowInternal(cairo_t *cr) {
    // TODO: Instabilities (?).
    static int counter = 0;
    if (counter * DO_CAIRO_DRAW_EVENT_TIME < 1.5) {
        counter++;
        return;
    }
    if (Flags.shutdownRequested) {
        return;
    }

    // Do all module clears.
    XFlush(mGlobal.display);

    eraseStarsFrame();
    removeAllStormItemsInItemset();

    XFlush(mGlobal.display);

    // Do cairo.
    cairo_save(cr);

    int tx = 0;
    int ty = 0;
    if (mGlobal.isCairoAvailable) {
        tx = mGlobal.StormWindowX;
        ty = mGlobal.StormWindowY;
    }
    cairo_translate(cr, tx, ty);

    // Do all module draws.
    if (WorkspaceActive()) {
        drawStarsFrame(cr);
        cairoDrawAllFallenItems(cr);
        drawAllStormItemsInItemset(cr);
    }

    cairo_restore(cr);
    XFlush(mGlobal.display);
}

/** *********************************************************************
 ** This method ...
 **/
int handleDisplayReconfigurationChange() {
    if (Flags.shutdownRequested) {
        return FALSE;
    }

    if (!mMainWindowNeedsReconfiguration) {
        return TRUE;
    }
    mMainWindowNeedsReconfiguration = false;

    if (!mGlobal.isStormWindowTransparent) {
        handleX11CairoDisplay();
    }

    if (mPrevStormWindowWidth != mGlobal.StormWindowWidth ||
        mPrevStormWindowHeight != mGlobal.StormWindowHeight) {
        updateDisplayDimensions();
        RestartDisplay();
        mPrevStormWindowWidth = mGlobal.StormWindowWidth;
        mPrevStormWindowHeight = mGlobal.StormWindowHeight;
        setStormWindowScale();
    }

    fflush(stdout);
    return TRUE;
}

/** *********************************************************************
 ** This method ...
 **/
int drawTransparentWindow(gpointer widget) {
    if (Flags.shutdownRequested) {
        return FALSE;
    }

    // This will result in a call off
    // handleTransparentWindowDrawEvents():
    gtk_widget_queue_draw(GTK_WIDGET(widget));
    return TRUE;
}

/** *********************************************************************
 ** This method handles callbacks for cpufactor
 **/
void HandleCpuFactor() {
    if (Flags.CpuLoad <= 0) {
        mGlobal.cpufactor = 1;
    } else {
        mGlobal.cpufactor = 100.0 / Flags.CpuLoad;
    }

    addMethodToMainloop(PRIORITY_HIGH,
        DO_STALL_CREATE_STORMITEM_EVENT_TIME,
        doStallCreateStormShapeEvent);

    addWindowDrawMethodToMainloop();
}

/** *********************************************************************
 ** This method...
 **/
void addWindowDrawMethodToMainloop() {
    if (mGlobal.isStormWindowTransparent) {
        if (mTransparentWindowGUID) {
            g_source_remove(mTransparentWindowGUID);
        }

        mTransparentWindowGUID = addMethodWithArgToMainloop(
            PRIORITY_HIGH, DO_CAIRO_DRAW_EVENT_TIME,
            drawTransparentWindow, mGlobal.gtkStormWindowWidget);
        return;
    }

    // If storm window not transparent.
    if (mGlobal.cairoWindowGuid) {
        g_source_remove(mGlobal.cairoWindowGuid);
    }

    mGlobal.cairoWindowGuid = addMethodWithArgToMainloop(
        PRIORITY_HIGH, DO_CAIRO_DRAW_EVENT_TIME,
        drawCairoWindow, mGlobal.cairoWindow);
}

/** *********************************************************************
 ** This method ...
 **/
void mybindtestdomain() {
    // One would assume that gettext() uses the environment variable LOCPATH
    // to find locales, but no, it does not.
    // So, we scan LOCPATH for paths, use them and see if gettext gets a
    // translation for a text that is known to exist in the ui and should be
    // translated to something different.
    #ifdef HAVE_GETTEXT

        char* startlocale;
        (void) startlocale;

        char* resultLocale = setlocale(LC_ALL, "");
        startlocale = strdup(resultLocale ? resultLocale : NULL);

        textdomain(PLASMASTORM_TEXT_DOMAIN);
        bindtextdomain(PLASMASTORM_TEXT_DOMAIN, LOCALEDIR);

        char* locpath = getenv("LOCPATH");
        if (locpath) {
            char *initial_textdomain = strdup(bindtextdomain(PLASMASTORM_TEXT_DOMAIN, NULL));
            char *mylocpath = strdup(locpath);
            int translation_found = False;

            char *q = mylocpath;
            while (1) {
                char *p = strsep(&q, ":");
                if (!p) {
                    break;
                }

                bindtextdomain(PLASMASTORM_TEXT_DOMAIN, p);
                if (strcmp(_(TEXT_DOMAIN_TEST_STRING),
                    TEXT_DOMAIN_TEST_STRING)) {
                    translation_found = True;
                    break;
                }
            }
            if (!translation_found) {
                bindtextdomain(PLASMASTORM_TEXT_DOMAIN,
                    initial_textdomain);
            }

            free(mylocpath);
            free(initial_textdomain);
        }
    #endif
}

/** *********************************************************************
 ** Set BelowAll. To be sure, we push down the Transparent GTK Window
 **               and our actual app Storm Window.
 **/
void setAppBelowAllWindows() {
    logAllWindowsStackedTopToBottom();
}
