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
#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

// Std C Lib headers.
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
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
#include "Blowoff.h"
#include "ClockHelper.h"
#include "Fallen.h"
#include "loadmeasure.h"
#include "mainstub.h"
#include "MainWindow.h"
#include "mygettext.h"
#include "plasmastorm.h"
#include "Prefs.h"
#include "rootWindowHelper.h"
#include "safeMalloc.h"
#include "Stars.h"
#include "Storm.h"
#include "transwindow.h"
#include "utils.h"
#include "versionHelper.h"
#include "Wind.h"
#include "windows.h"
#include "x11WindowHelper.h"
#include "xdo.h"


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

static void HandleCpuFactor();

static void RestartDisplay();
static void SigHandler(int);

static int handleX11ErrorEvent(Display*, XErrorEvent*);
static int drawCairoWindow(void*);
static void handleX11CairoDisplay();

static void addWindowDrawMethodToMainloop();

static void drawCairoWindowInternal(cairo_t*);

static int drawTransparentWindow(gpointer);

static gboolean handleTransparentWindowDrawEvents(
    GtkWidget*, cairo_t*, gpointer);

static void StartWindow();
static void SetWindowScale();

static int handlePendingX11Events();

static void mybindtestdomain();

static void DoAllWorkspaces();
extern void setTransparentWindowAbove(GtkWindow* window);
extern void setAppAboveOrBelowAllWindows();
static void setmGlobalDesktopSession();

extern int updateWindowsList();
extern void getWinInfoList();

static int doAllUISettingsUpdates();
void respondToAdvancedSettingsChanges();

static int handleDisplayReconfigurationChange();

void uninitQPickerDialog();


/** *********************************************************************
 ** Module globals and consts.
 **/
//static char **mArgV;
//static int mArgC;

struct _mGlobal mGlobal;

Bool mMainWindowNeedsReconfiguration = true;

static char* mStormWindowTItle = NULL;
static guint mTransparentWindowGUID = 0;
static guint mCairoWindowGUID = 0;
static GtkWidget *mTransparentGTKWindow = NULL;

static bool mX11CairoEnabled = false;
cairo_t *mCairoWindow = NULL;
cairo_surface_t *mCairoSurface = NULL;

int xfixes_event_base_ = -1;

static int mIsSticky = 0;

static int wantx = 0;
static int wanty = 0;

static int mPrevStormWindowWidth = 0;
static int mPrevStormWindowHeight = 0;

static int mX11ErrorCount = 0;
const int mX11MaxErrorCount = 500;

 

/** *********************************************************************
 ** Application start method. 
 **/
int startApplication(int argc, char *argv[]) {
    logAppVersion();

    printf("Available languages are: %s.\n\n",
        LANGUAGES);

    // Log Gnome info.
    printf("GTK version: %s\n", ui_gtk_version());
    #ifdef GSL_VERSION
        fprintf(stdout, "GSL version: %s\n", GSL_VERSION);
    #else
        fprintf(stdout, "GSL version: UNKNOWN\n");
    #endif

    if (!isGtkVersionValid()) {
        printf("plasmastorm needs gtk version >= %s, "
            "found version %s \n", ui_gtk_required(),
            ui_gtk_version());
        return 0;
    }

    // Circumvent wayland problems. Before starting GTK,
    // ensure that the gdk-x11 backend is used.
    setenv("GDK_BACKEND", "x11", 1);
    mGlobal.isWaylandDisplay = getenv("WAYLAND_DISPLAY") &&
        getenv("WAYLAND_DISPLAY")[0];
    fprintf(stdout, "\nWayland desktop %s detected.\n",
        mGlobal.isWaylandDisplay ? "was" : "was not");

    // Set app shutdown handlers.
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);
    signal(SIGHUP, SigHandler);

    // Seed random generator.
    srand48((int) (fmod(wallcl() * 1.0e6, 1.0e8)));

    // Clear space for app mGlobal struct.
    memset(&mGlobal, 0, sizeof(mGlobal));

    mGlobal.DesktopSession = NULL;
    mGlobal.isCompizCompositor = false;
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

    mGlobal.WindowOffsetWindowTops = 0;
    mGlobal.WindowOffsetX = 0;
    mGlobal.MaxDesktopFallenDepth = 0;
    mGlobal.FallenFirst = NULL;

    mGlobal.cpufactor = 1.0;
    mGlobal.WindowScale = 1.0;

    mGlobal.NVisWorkSpaces = 1;
    mGlobal.CWorkSpace = 0;
    mGlobal.ChosenWorkSpace = 0;
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
        printf("plasmastorm needs xdo and it reports "
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

    StartWindow();
    printf("\nIt\'s Storming in [%#lx] :  %s\n   xPos: %d  yPos: %d\n   "
        "width: %d  heigth: %d\n", mGlobal.StormWindow, mStormWindowTItle,
        mGlobal.StormWindowX, mGlobal.StormWindowY, mGlobal.StormWindowWidth,
        mGlobal.StormWindowHeight);

    // Init all Global Flags.
    OldFlags.Done = Flags.Done;
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
    printf("plasmastorm: startApplication() Value: ShowStars %i.\n",
        Flags.ShowStars);
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

    initializeMainWindow();

    ui_set_sticky(Flags.AllWorkspaces);

    Flags.Done = 0;
    updateWindowsList();

    // Init app modules.
    addWindowsModuleToMainloop();

    initWindModule();
    initBlowoffModule();
    initFallenModule();
    initStarsModule();
    initStormModule();

    addLoadMonitorToMainloop();

    addMethodToMainloop(PRIORITY_DEFAULT, DO_HANDLE_X11_EVENT_TIME,
        handlePendingX11Events);

    addMethodToMainloop(PRIORITY_DEFAULT, DO_DISPLAY_RECONFIGURATION_EVENT_TIME,
        handleDisplayReconfigurationChange);

    addMethodToMainloop(PRIORITY_HIGH, DO_UI_SETTINGS_UPDATES_EVENT_TIME,
        doAllUISettingsUpdates);

    HandleCpuFactor();
    fflush(stdout);

    // Set mGlobal.ChosenWorkSpace then main loop.
    DoAllWorkspaces();

    // Gopher it!
    gtk_main();

    // Terminate it!
    if (mStormWindowTItle) {
        free(mStormWindowTItle);
    }

    XClearWindow(mGlobal.display, mGlobal.StormWindow);
    XFlush(mGlobal.display);
    XCloseDisplay(mGlobal.display);
    uninitQPickerDialog();

    // If Restarting due to display or language change.
    if (mGlobal.languageChangeRestart) {
        sleep(0);
        setenv("plasmastorm_RESTART", "yes", 1);
        execvp(argv[0], argv);
        return 0;
    }

    fprintf(stdout, "\nThanks for using plasmastorm!"
        " - You\'re the best!\n");
    fflush(stdout);

    return 0;
}

/** *********************************************************************
 ** Set the app x11 window order based on user pref.
 **/
void setAppAboveOrBelowAllWindows() {
    setAppBelowAllWindows();
}

/** *********************************************************************
 ** This method ...
 **/
void StartWindow() {
    mGlobal.Rootwindow = DefaultRootWindow(mGlobal.display);

    mGlobal.hasDestopWindow = false;
    mGlobal.isStormWindowTransparent = false;

    // Try to create a transparent clickthrough window.
    GtkWidget* gtkWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_widget_set_can_focus(gtkWindow, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(gtkWindow), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(gtkWindow),
        GDK_WINDOW_TYPE_HINT_POPUP_MENU);

    Window transparentGTKWindow;
    if (createTransparentWindow(mGlobal.display, gtkWindow,
        Flags.AllWorkspaces, true, true, NULL,
        &transparentGTKWindow, &wantx, &wanty)) {

        mTransparentGTKWindow = gtkWindow;
        mGlobal.StormWindow = transparentGTKWindow;
        mGlobal.isStormWindowTransparent = true;

        mGlobal.hasDestopWindow = true;

        GtkWidget *drawing_area = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(mTransparentGTKWindow), drawing_area);

        g_signal_connect(mTransparentGTKWindow, "draw",
            G_CALLBACK(handleTransparentWindowDrawEvents), NULL);
    } else {
        setmGlobalDesktopSession();

        // Find global StormWindow based on desktop.
        mGlobal.hasDestopWindow = true;
        mX11CairoEnabled = true;

        Window usableStormWindow = None;
        if (!strncmp(mGlobal.DesktopSession, "LXDE", 4)) {
            usableStormWindow = largest_window_with_name(mGlobal.xdo, "^pcmanfm$");
        }
        if (!usableStormWindow) {
            usableStormWindow = largest_window_with_name(mGlobal.xdo, "^Desktop$");
        }
        if (!usableStormWindow) {
            usableStormWindow = mGlobal.Rootwindow;
        }
        mGlobal.StormWindow = usableStormWindow;
    }

    // Start window Cairo specific.
    if (mX11CairoEnabled) {
        handleX11CairoDisplay();
        mCairoWindowGUID = addMethodWithArgToMainloop(PRIORITY_HIGH,
            DO_CAIRO_DRAW_EVENT_TIME, drawCairoWindow, mCairoWindow);
        mGlobal.WindowOffsetX = 0;
        mGlobal.WindowOffsetWindowTops = 0;
    } else {
        mGlobal.WindowOffsetX = wantx;
        mGlobal.WindowOffsetWindowTops = wanty;
    }

    mStormWindowTItle = strdup("no name");
    XTextProperty titleBarName;
    if (XGetWMName(mGlobal.display, mGlobal.StormWindow, &titleBarName)) {
        mStormWindowTItle = strdup((char *) titleBarName.value);
    }
    XFree(titleBarName.value);

    if (!mX11CairoEnabled) {
        xdo_move_window(mGlobal.xdo, mGlobal.StormWindow, wantx, wanty);
    }

    xdo_wait_for_window_map_state(mGlobal.xdo, mGlobal.StormWindow, IsViewable);

    initDisplayDimensions();
    mGlobal.StormWindowX = wantx;
    mGlobal.StormWindowY = wanty;
    mPrevStormWindowWidth = mGlobal.StormWindowWidth;
    mPrevStormWindowHeight = mGlobal.StormWindowHeight;

    fflush(stdout);
    SetWindowScale();
}

/** *********************************************************************
 ** 
 **/
void setmGlobalDesktopSession() {
    // Early out if already set.
    if (mGlobal.DesktopSession != NULL) {
        return;
    }

    const char *DESKTOPS[] = {"DESKTOP_SESSION",
        "XDG_SESSION_DESKTOP", "XDG_CURRENT_DESKTOP",
        "GDMSESSION", NULL};

    for (int i = 0; DESKTOPS[i]; i++) {
        char* desktopsession = NULL;
        desktopsession = getenv(DESKTOPS[i]);
        if (desktopsession) {
            mGlobal.DesktopSession = strdup(desktopsession);
            return;
        }
    }

    mGlobal.DesktopSession =
        (char *) "unknown_desktop_session";
}

/** *********************************************************************
 ** Cairo specific. handleDisplayReconfigurationChange()
 ** startApplication->StartWindow()
 **/
void handleX11CairoDisplay() {
    unsigned int width, height;
    xdo_get_window_size(mGlobal.xdo, mGlobal.StormWindow,
        &width, &height);

    printf("handleX11CairoDisplay() Double buffers unavailable "
        "or unrequested.\n");
    Visual* visual = DefaultVisual(mGlobal.display,
        DefaultScreen(mGlobal.display));
    mCairoSurface = cairo_xlib_surface_create(mGlobal.display,
        mGlobal.StormWindow, visual, width, height);

    // Destroy & create new Cairo Window.
    if (mCairoWindow) {
        cairo_destroy(mCairoWindow);
    }
    mCairoWindow = cairo_create(mCairoSurface);
    cairo_xlib_surface_set_size(mCairoSurface, width, height);

    mGlobal.StormWindowWidth = width;
    mGlobal.StormWindowHeight = height;
}

/** *********************************************************************
 ** Set the Transparent Window Sticky Flag.
 **/
void setTransparentWindowStickyState(int isSticky) {
    if (!mGlobal.isStormWindowTransparent) {
        return;
    }

    mIsSticky = isSticky;
    if (mIsSticky) {
        gtk_window_stick(GTK_WINDOW(mTransparentGTKWindow));
    } else {
        gtk_window_unstick(GTK_WINDOW(mTransparentGTKWindow));
    }
}

/** *********************************************************************
 ** TODO:
 **/
void DoAllWorkspaces() {
    if (Flags.AllWorkspaces) {
        setTransparentWindowStickyState(1);
    } else {
        setTransparentWindowStickyState(0);
        mGlobal.ChosenWorkSpace = mGlobal.workspaceArray[0];
    }

    ui_set_sticky(Flags.AllWorkspaces);
}

/** *********************************************************************
 * here we are handling the buttons in ui
 * Ok, this is a long list, and could be implemented more efficient.
 * But, doAllUISettingsUpdates is called not too frequently, so....
 * Note: if changes != 0, the settings will be written to .plasmastormrc
 **/
int doAllUISettingsUpdates() {
    if (Flags.Done) {
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
        DoAllWorkspaces();
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
 ** This method handles xlib11 event notifications.
 ** Undergoing heavy renovation 2024 Rocks !
 **
 ** https://www.x.org/releases/current/doc/libX11/libX11/libX11.html#Events
 **/
int handlePendingX11Events() {
    if (Flags.Done) {
        return FALSE;
    }

    XFlush(mGlobal.display);
    while (XPending(mGlobal.display)) {
        XEvent event;
        XNextEvent(mGlobal.display, &event);

        // Check for Active window change through loop.
        const Window activeX11Window = getActiveX11Window();
        if (getActiveAppWindow() != activeX11Window) {
            onAppWindowChange(activeX11Window);
        }

        // Perform X11 event action.
        switch (event.type) {
            case CreateNotify:
                onWindowCreated(&event);
                break;

            case ReparentNotify:
                onWindowReparent(&event);
                break;

            case ConfigureNotify:
                onWindowChanged(&event);
                if (!isWindowBeingDragged()) {
                    mGlobal.windowsWereDraggedOrMapped++;
                    if (event.xconfigure.window == mGlobal.StormWindow) {
                        mMainWindowNeedsReconfiguration = true;
                    }
                }
                break;

            case MapNotify:
                mGlobal.windowsWereDraggedOrMapped++;
                onWindowMapped(&event);
                break;

            case FocusIn:
                onWindowFocused(&event);
                break;

            case FocusOut:
                onWindowBlurred(&event);
                break;

            case UnmapNotify:
                mGlobal.windowsWereDraggedOrMapped++;
                onWindowUnmapped(&event);
                break;

            case DestroyNotify:
                onWindowDestroyed(&event);
                break;

            default:
                // Perform XFixes action.
                int xfixes_event_base;
                int xfixes_error_base;
                if (XFixesQueryExtension(mGlobal.display,
                    &xfixes_event_base, &xfixes_error_base)) {
                    switch (event.type - xfixes_event_base) {
                        case XFixesCursorNotify:
                            onCursorChange(&event);
                            break;
                    }
                }
                break;
        }
    }

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
 ** This method ...
 **/
void SigHandler(__attribute__((unused)) int signum) {
    Flags.Done = 1;
}

/** *********************************************************************
 ** This method traps and handles X11 errors.
 **
 ** Primarily, we close the app if the system doesn't seem sane.
 **/
int handleX11ErrorEvent(Display* dpy, XErrorEvent* err) {

    const int MAX_MESSAGE_BUFFER_LENGTH = 60;
    char msg[MAX_MESSAGE_BUFFER_LENGTH];

    XGetErrorText(dpy, err->error_code, msg, sizeof(msg));
    fprintf(stdout, "%s", msg);

    if (mX11ErrorCount++ > mX11MaxErrorCount) {
        fprintf(stdout, "More than %d errors, I quit!",
            mX11MaxErrorCount);
        Flags.Done = 1;
    }

    return 0;
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
    if (Flags.Done) {
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
    if (mX11CairoEnabled) {
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
void SetWindowScale() {
    // assuming a standard screen of 1024x576, we suggest to use the scalefactor
    // WindowScale
    float x = mGlobal.StormWindowWidth / 1000.0;
    float y = mGlobal.StormWindowHeight / 576.0;

    if (x < y) {
        mGlobal.WindowScale = x;
    } else {
        mGlobal.WindowScale = y;
    }
}

/** *********************************************************************
 ** This method ...
 **/
int handleDisplayReconfigurationChange() {
    if (Flags.Done) {
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
        SetWindowScale();
    }

    fflush(stdout);
    return TRUE;
}

/** *********************************************************************
 ** This method ...
 **/
int drawTransparentWindow(gpointer widget) {
    if (Flags.Done) {
        return FALSE;
    }

    // this will result in a call off handleTransparentWindowDrawEvents():
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

    addMethodToMainloop(PRIORITY_HIGH, DO_STALL_CREATE_STORMITEM_EVENT_TIME,
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
        mTransparentWindowGUID = addMethodWithArgToMainloop(PRIORITY_HIGH,
            DO_CAIRO_DRAW_EVENT_TIME, drawTransparentWindow, mTransparentGTKWindow);
        return;
    }

    if (mCairoWindowGUID) {
        g_source_remove(mCairoWindowGUID);
    }

    mCairoWindowGUID = addMethodWithArgToMainloop(PRIORITY_HIGH,
        DO_CAIRO_DRAW_EVENT_TIME, drawCairoWindow, mCairoWindow);
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
