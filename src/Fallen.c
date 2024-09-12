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
#include "config.h"
#endif

#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_spline.h>

#include <gtk/gtk.h>

#include "Blowoff.h"
#include "Fallen.h"
#include "Prefs.h"
#include "safeMalloc.h"
#include "Storm.h"
#include "splineHelper.h"
#include "utils.h"
#include "Wind.h"
#include "Windows.h"

/** *********************************************************************
 ** Module globals and consts.
 **/

#define MAX_SPLINES_PER_FALLEN 6

#define MINIMUM_SPLINE_WIDTH 3

// Semaphore & lock members.
static sem_t mFallenSwapSemaphore;
static sem_t mFallenBaseSemaphore;

/***********************************************************
 * Helper methods for semaphores.
 */
void initFallenSemaphores() {
    sem_init(&mFallenSwapSemaphore, 0, 1);
    sem_init(&mFallenBaseSemaphore, 0, 1);
}

// Swap semaphores.
int lockFallenSwapSemaphore() {
    return sem_wait(&mFallenSwapSemaphore);
}
int unlockFallenSwapSemaphore() {
    return sem_post(&mFallenSwapSemaphore);
}

// Base semaphores.
int lockFallenBaseSemaphore() {
    return sem_wait(&mFallenBaseSemaphore);
}
int unlockFallenBaseSemaphore() {
    return sem_post(&mFallenBaseSemaphore);
}
int softLockFallenBaseSemaphore(
    int maxSoftTries, int* tryCount) {
    int resultCode;

    // Guard tryCount and bump.
    if (*tryCount < 0) {
        *tryCount = 0;
    }
    (*tryCount)++;

    // Set resultCode from soft or hard wait.
    resultCode = (*tryCount > maxSoftTries) ?
        sem_wait(&mFallenBaseSemaphore) :
        sem_trywait(&mFallenBaseSemaphore);

    // Success clears tryCount for next time.
    if (resultCode == 0) {
        *tryCount = 0;
    }

    return resultCode;
}

/** *********************************************************************
 ** This method ...
 **/
void initFallenModule() {
    initFallenListWithDesktop();

    addMethodToMainloop(PRIORITY_DEFAULT,
        time_change_bottom, do_change_deshes);
    addMethodToMainloop(PRIORITY_DEFAULT,
        time_adjust_bottom, do_adjust_deshes);

    static pthread_t thread;
    pthread_create(&thread, NULL, execFallenThread, NULL);
}

/** *********************************************************************
 ** This method clears and inits the FallenItem list of items.
 **/
void initFallenListWithDesktop() {
    lockFallenSemaphore();

    // Pop all off list.
    while (mGlobal.FallenFirst) {
        popFallenItem(&mGlobal.FallenFirst);
    }

    // Create FallenItem item for bottom of screen.
    // Add to list as first entry.
    WinInfo* tempWindow = (WinInfo*) malloc(sizeof(WinInfo));
    memset(tempWindow, 0, sizeof(WinInfo));
    pushFallenItem(&mGlobal.FallenFirst, tempWindow,
        0, mGlobal.StormWindowHeight, mGlobal.StormWindowWidth,
        mGlobal.MaxDesktopFallenDepth);
    free(tempWindow);

    unlockFallenSemaphore();
}

/** *********************************************************************
 ** This method is a private thread looper.
 **/
void* execFallenThread() {
    // Loop until cancelled.
    while (1) {
        if (Flags.shutdownRequested) {
            pthread_exit(NULL);
        }

        // Main thread method.
        updateAllFallenOnThread();

        usleep((useconds_t)
            DO_FALLEN_THREAD_EVENT_TIME * 1000000);
    }

    return NULL;
}


/** *********************************************************************
 ** This method ...
 **/
void updateAllFallenOnThread() {
    if (!WorkspaceActive() || !Flags.ShowStormItems) {
        return;
    }
     if (!Flags.KeepFallenOnWindows && !Flags.KeepFallenOnDesktop) {
        return;
    }

    lockFallenSemaphore();

    FallenItem* fallen = mGlobal.FallenFirst;
    while (fallen) {
        if (canFallenConsumeStormItem(fallen)) {
            drawFallenItem(fallen);
        }
        fallen = fallen->next;
    }

    XFlush(mGlobal.display);
    swapFallenListItemSurfaces();
    unlockFallenSemaphore();
}

/** *********************************************************************
 ** This method ...
 **/
void respondToSurfacesSettingsChanges() {
    //UIDO(KeepFallenOnWindows,
    if (Flags.KeepFallenOnWindows !=
            OldFlags.KeepFallenOnWindows) {
        OldFlags.KeepFallenOnWindows =
            Flags.KeepFallenOnWindows;
        initFallenListWithDesktop();
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(MaxWindowFallenDepth,
    if (Flags.MaxWindowFallenDepth !=
            OldFlags.MaxWindowFallenDepth) {
        OldFlags.MaxWindowFallenDepth =
            Flags.MaxWindowFallenDepth;
        initFallenListWithDesktop();
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(WindowFallenTopOffset, 
    if (Flags.WindowFallenTopOffset !=
            OldFlags.WindowFallenTopOffset) {
        OldFlags.WindowFallenTopOffset =
            Flags.WindowFallenTopOffset;
        updateFallenRegionsWithLock();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(KeepFallenOnDesktop,
    if (Flags.KeepFallenOnDesktop !=
            OldFlags.KeepFallenOnDesktop) {
        OldFlags.KeepFallenOnDesktop =
            Flags.KeepFallenOnDesktop;
        initFallenListWithDesktop();
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(MaxDesktopFallenDepth,
    if (Flags.MaxDesktopFallenDepth !=
            OldFlags.MaxDesktopFallenDepth) {
        OldFlags.MaxDesktopFallenDepth =
            Flags.MaxDesktopFallenDepth;
        setMaxDesktopFallenDepth();
        initFallenListWithDesktop();
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(DesktopFallenTopOffset, 
    if (Flags.DesktopFallenTopOffset !=
            OldFlags.DesktopFallenTopOffset) {
        OldFlags.DesktopFallenTopOffset =
            Flags.DesktopFallenTopOffset;
        updateDisplayDimensions();
        Flags.mHaveFlagsChanged++;
    }
}

/** *********************************************************************
 ** This method ...
 **/
void boundMaxDesktopFallenDepth() {
    // Must stay storm free on somw part of display :)
    #define KEEP_FREE 25

    mGlobal.MaxDesktopFallenDepth =
        Flags.MaxDesktopFallenDepth;

    const int r = mGlobal.StormWindowHeight - KEEP_FREE;
    if (mGlobal.MaxDesktopFallenDepth > r) {
        mGlobal.MaxDesktopFallenDepth = r;
    }
}

/** *********************************************************************
 ** This method ...
 **/
void setMaxDesktopFallenDepth() {
    lockFallenSemaphore();
    boundMaxDesktopFallenDepth();
    unlockFallenSemaphore();
}

/** *********************************************************************
 ** This method ...
 **/
int canFallenConsumeStormItem(FallenItem *fallen) {
    if (fallen->winInfo.window == None) {
        return Flags.KeepFallenOnDesktop;
    }

    if (fallen->winInfo.hidden) {
        return false;
    }

    if (!fallen->winInfo.sticky &&
        !isFallenOnVisibleWorkspace(fallen)) {
        return false;
    }

    return Flags.KeepFallenOnWindows;
}

/** *********************************************************************
 ** This method ...
 **/
int isFallenOnVisibleWorkspace(FallenItem *fallen) {
    if (fallen) {
        for (int i = 0; i < mGlobal.visibleWorkspaceCount; i++) {
            if (mGlobal.workspaceArray[i] == fallen->winInfo.ws) {
                return true;
            }
        }
    }

    return false;
}

/** *********************************************************************
 ** Animation of blowoff.
 **
 ** This method removes some fallen StormItems from fallen, w pixels.
 **
 ** Also add storm items.
 **/
void updateFallenWithWind(FallenItem *fallen, int w, int h) {
    int x = randint(fallen->w - w);

    for (int i = x; i < x + w; i++) {
        if (fallen->fallenHeight[i] > h) {
            if (Flags.ShowWind && mGlobal.Wind != 0 && drand48() > 0.5) {

                const int numberOfItemsToMake =
                    getBlowoffEventCount();
                for (int j = 0; j < numberOfItemsToMake; j++) {
                    StormItem* stormItem = createStormItem(
                        Flags.ComboStormShape - 1);

                    stormItem->xRealPosition = fallen->x + i;
                    stormItem->yRealPosition = fallen->y -
                        fallen->fallenHeight[i] - drand48() * 4;
                    stormItem->xVelocity = 0.25 *
                        getWindDirection(mGlobal.NewWind) * mGlobal.WindMax;
                    stormItem->yVelocity = -10;

                    // Not cyclic for Windows, cyclic for bottom.
                    stormItem->cyclic = (fallen->winInfo.window == 0);
                }

                eraseFallenAtPixel(fallen, i);
            }
        }
    }
}

/** *********************************************************************
 ** This method ...
 **/
void updateFallenPartial(FallenItem* fallen, int position, int width) {
    if (!WorkspaceActive() || !Flags.ShowStormItems) {
        return;
    }
    if (!Flags.KeepFallenOnWindows && !Flags.KeepFallenOnDesktop) {
        return;
    }

    if (!canFallenConsumeStormItem(fallen)) {
        return;
    }

    // tempHeightArray will contain the fallenHeight values
    // corresponding with position-1 .. position+width (inclusive).
    short int* tempHeightArray;
    tempHeightArray = (short int*)
        malloc(sizeof(* tempHeightArray) * (width + 2));

    //
    int imin = position;
    if (imin < 0) {
        imin = 0;
    }
    int imax = position + width;
    if (imax > fallen->w) {
        imax = fallen->w;
    }
    int k = 0;
    for (int i = imin - 1; i <= imax; i++) {
        if (i < 0) {
            tempHeightArray[k++] = fallen->fallenHeight[0];
        } else if (i >= fallen->w) {
            tempHeightArray[k++] = fallen->fallenHeight[fallen->w - 1];
        } else {
            tempHeightArray[k++] = fallen->fallenHeight[i];
        }
    }

    // Raise FallenItem values.
    int amountToRaiseHeight;
    if (fallen->fallenHeight[imin] < fallen->maxFallenHeight[imin] / 4) {
        amountToRaiseHeight = 4;
    } else if (fallen->fallenHeight[imin] < fallen->maxFallenHeight[imin] / 2) {
        amountToRaiseHeight = 2;
    } else {
        amountToRaiseHeight = 1;
    }

    k = 1;
    for (int i = imin; i < imax; i++) {
        if ((fallen->maxFallenHeight[i] > tempHeightArray[k]) &&
            (tempHeightArray[k - 1] >= tempHeightArray[k] ||
                tempHeightArray[k + 1] >= tempHeightArray[k])) {
            fallen->fallenHeight[i] = amountToRaiseHeight +
                (tempHeightArray[k - 1] + tempHeightArray[k + 1]) / 2;
        }
        k++;
    }

    // tempHeightArray will contain the new fallenHeight values
    // corresponding with position-1..position+width.
    k = 0;
    for (int i = imin - 1; i <= imax; i++) {
        if (i < 0) {
            tempHeightArray[k++] = fallen->fallenHeight[0];
        } else if (i >= fallen->w) {
            tempHeightArray[k++] = fallen->fallenHeight[fallen->w - 1];
        } else {
            tempHeightArray[k++] = fallen->fallenHeight[i];
        }
    }

    // And now some smoothing.
    k = 1;
    for (int i = imin; i < imax; i++) {
        int sum = 0;
        for (int j = k - 1; j <= k + 1; j++) {
            sum += tempHeightArray[j];
        }
        fallen->fallenHeight[i] = sum / 3;
        k++;
    }

    free(tempHeightArray);
}

/** *********************************************************************
 ** This method ...
 **/
void updateFallenAtBottom() {
    // threads: locking by caller
    FallenItem *fallen = findFallenItemByWindow(
        mGlobal.FallenFirst, 0);
    if (fallen) {
        fallen->y = mGlobal.StormWindowHeight;
    }
}

/** *********************************************************************
 ** This method ... threads: locking by caller
 **/
void generateFallenStormItems(FallenItem *fallen,
    int x, int w, float yVelocity) {
    if (!Flags.ShowBlowoff || !Flags.ShowStormItems) {
        return;
    }

    int ifirst = x;
    if (ifirst < 0) {
        ifirst = 0;
    }
    if (ifirst > fallen->w) {
        ifirst = fallen->w;
    }
    int ilast = x + w;
    if (ilast < 0) {
        ilast = 0;
    }
    if (ilast > fallen->w) {
        ilast = fallen->w;
    }

    for (int i = ifirst; i < ilast; i += 2) {
        for (int j = 0; j < fallen->fallenHeight[i]; j += 2) {

            const int kmax = getBlowoffEventCount();
            for (int k = 0; k < kmax; k++) {
                const bool probability = (drand48() < 0.1);

                if (probability) {
                    StormItem* stormItem = createStormItem(
                        Flags.ComboStormShape - 1);

                    stormItem->cyclic = 0;

                    stormItem->xRealPosition = fallen->x + i + 16 *
                        (drand48() - 0.5);
                    stormItem->yRealPosition = fallen->y - j - 8;

                    stormItem->xVelocity = (Flags.ShowWind) ?
                        mGlobal.NewWind / 8 : 0;
                    stormItem->yVelocity = yVelocity;
                }
            }
        }
    }
}

/** *********************************************************************
 ** This method pushes a node onto the list.
 **/
void pushFallenItem(FallenItem** fallenArray,
    WinInfo* window, int x, int y, int w, int h) {

    // TODO: "Too-narrow" windows
    // results in complications.
    if (w < MINIMUM_SPLINE_WIDTH) {
        return;
    }

    // Allocate new object.
    FallenItem* fallenListItem = (FallenItem*)
        malloc(sizeof(FallenItem));

    fallenListItem->winInfo = *window;

    fallenListItem->x = x;
    fallenListItem->y = y;
    fallenListItem->w = w;
    fallenListItem->h = h;

    fallenListItem->prevx = 0;
    fallenListItem->prevy = 0;
    fallenListItem->prevw = 10;
    fallenListItem->prevh = 10;

    fallenListItem->surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, w, h);
    fallenListItem->surface1 = cairo_surface_create_similar(
        fallenListItem->surface, CAIRO_CONTENT_COLOR_ALPHA,
        w, h);

    // Allocate arrays.
    fallenListItem->columnColor   = (GdkRGBA *)
        malloc(sizeof(*(fallenListItem->columnColor)) * w);
    fallenListItem->fallenHeight    = (short int *)
        malloc(sizeof(*(fallenListItem->fallenHeight)) * w);
    fallenListItem->maxFallenHeight = (short int *)
        malloc(sizeof(*(fallenListItem->maxFallenHeight)) * w);

    // Fill arrays.
    for (int i = 0; i < w; i++) {
        fallenListItem->columnColor[i] =
            getNextStormShapeColorAsRGB();
        fallenListItem->fallenHeight[i] = 0;
        fallenListItem->maxFallenHeight[i] = h;
    }

    CreateDesh(fallenListItem);

    fallenListItem->next = *fallenArray;
    *fallenArray = fallenListItem;
}

/** *********************************************************************
 ** This method pops a node from the start of the list.
 **/
void popFallenItem(FallenItem **list) {
    if (*list == NULL) {
        return;
    }

    FallenItem* next_node = (*list)->next;
    freeFallenItemMemory(*list);

    *list = next_node;
}

/** *********************************************************************
 ** This method ...
 ** threads: locking by caller
 **/
void CreateDesh(FallenItem *p) {
    int w = p->w;
    int h = p->h;

    int id = p->winInfo.window;
    short int *maxFallenHeight = p->maxFallenHeight;

    double splinex[MAX_SPLINES_PER_FALLEN];
    double spliney[MAX_SPLINES_PER_FALLEN];

    randomuniqarray(splinex, MAX_SPLINES_PER_FALLEN, 0.0000001, NULL);
    for (int i = 0; i < MAX_SPLINES_PER_FALLEN; i++) {
        splinex[i] *= (w - 1);
        spliney[i] = drand48();
    }

    splinex[0] = 0;
    splinex[MAX_SPLINES_PER_FALLEN - 1] = w - 1;
    if (id == 0) { // bottom
        spliney[0] = 1.0;
        spliney[MAX_SPLINES_PER_FALLEN - 1] = 1.0;
    } else {
        spliney[0] = 0;
        spliney[MAX_SPLINES_PER_FALLEN - 1] = 0;
    }

    double *x = (double *) malloc(w * sizeof(double));
    double *y = (double *) malloc(w * sizeof(double));
    for (int i = 0; i < w; i++) {
        x[i] = i;
    }

    spline_interpol(splinex, spliney, MAX_SPLINES_PER_FALLEN,
        x, y, w);

    for (int i = 0; i < w; i++) {
        maxFallenHeight[i] = h * y[i];
        if (maxFallenHeight[i] < 2) {
            maxFallenHeight[i] = 2;
        }
    }

    free(x);
    free(y);
}

/** *********************************************************************
 ** This method ...
 **/
// change to desired heights
int do_change_deshes() {
    static int lockcounter;
    if (softLockFallenBaseSemaphore(3, &lockcounter)) {
        return TRUE;
    }

    FallenItem *fallen = mGlobal.FallenFirst;
    while (fallen) {
        CreateDesh(fallen);
        fallen = fallen->next;
    }

    unlockFallenSemaphore();
    return TRUE;
}

/** *********************************************************************
 ** This method ...
 **/
int do_adjust_deshes(__attribute__((unused))void* dummy) {
    // TODO: threads: probably no need for
    // lock, but to be sure:
    lockFallenSemaphore();

    FallenItem *fallen = mGlobal.FallenFirst;
    while (fallen) {
        int adjustments = 0;

        for (int i = 0; i < fallen->w; i++) {
            int d = fallen->fallenHeight[i] - fallen->maxFallenHeight[i];
            if (d > 0) {
                int c = 1;
                adjustments++;
                fallen->fallenHeight[i] -= c;
            }
        }
        fallen = fallen->next;
    }

    unlockFallenSemaphore();
    return TRUE;
}

/** *********************************************************************
 ** This method ...
 **/
// threads: locking by caller
void createFallenDisplayArea(FallenItem* fallen) {
    cairo_t* cr = cairo_create(fallen->surface1);

    short int *fallenHeight = fallen->fallenHeight;

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgb(cr, fallen->columnColor[0].red,
        fallen->columnColor[0].green, fallen->columnColor[0].blue);

    // Clear surface1.
    cairo_save(cr);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
    cairo_restore(cr);

    // MAIN SPLINE adjustment loop.
    // Compute averages for 10 points, draw spline through them
    // and use that to draw fallen
    const int fallenItemWidth = fallen->w;
    const int fallenItemHeight = fallen->h;

    const int NUMBER_OF_POINTS_FOR_AVERAGE = 10;
    const int NUMBER_OF_AVERAGE_POINTS = MINIMUM_SPLINE_WIDTH +
        (fallenItemWidth - 2) / NUMBER_OF_POINTS_FOR_AVERAGE;

    double* averageHeightList = (double*)
        malloc(NUMBER_OF_AVERAGE_POINTS * sizeof(double));
    averageHeightList[0] = 0;

    double* averageXPosList  = (double*)
        malloc(NUMBER_OF_AVERAGE_POINTS * sizeof(double));
    averageXPosList[0] = 0;

    for (int i = 0; i < NUMBER_OF_AVERAGE_POINTS -
        MINIMUM_SPLINE_WIDTH; i++) {

        double averageHeight = 0;
        for (int j = 0; j < NUMBER_OF_POINTS_FOR_AVERAGE; j++) {
            averageHeight += fallenHeight[NUMBER_OF_POINTS_FOR_AVERAGE * i + j];
        }

        averageHeightList[i + 1] = averageHeight / NUMBER_OF_POINTS_FOR_AVERAGE;
        averageXPosList[i + 1] = NUMBER_OF_POINTS_FOR_AVERAGE * i +
            NUMBER_OF_POINTS_FOR_AVERAGE * 0.5;
    }

    // Desktop.
    if (fallen->winInfo.window == None) {
        averageHeightList[0] = averageHeightList[1];
    }

    // FINAL SPLINE adjustment loop.
    int k = NUMBER_OF_AVERAGE_POINTS - MINIMUM_SPLINE_WIDTH;
    int mk = NUMBER_OF_POINTS_FOR_AVERAGE * k;

    double averageHeight = 0;
    for (int i = mk; i < fallenItemWidth; i++) {
        averageHeight += fallenHeight[i];
    }

    averageHeightList[k + 1] = averageHeight / (fallenItemWidth - mk);
    averageXPosList[k + 1] = mk + 0.5 * (fallenItemWidth - mk - 1);

    // Desktop.
    if (fallen->winInfo.window == None) {
        averageHeightList[NUMBER_OF_AVERAGE_POINTS - 1] =
            averageHeightList[NUMBER_OF_AVERAGE_POINTS - 2];
    } else {
        averageHeightList[NUMBER_OF_AVERAGE_POINTS - 1] = 0;
    }
    averageXPosList[NUMBER_OF_AVERAGE_POINTS - 1] =
        fallenItemWidth - 1;

    cairo_set_line_width(cr, 1);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

    // GSL - GNU Scientific Library for graph draws.
    // Accelerator object, a kind of iterator.
    gsl_interp_accel* accelerator = gsl_interp_accel_alloc();
    gsl_spline* spline = gsl_spline_alloc(SPLINE_INTERP,
        NUMBER_OF_AVERAGE_POINTS);
    gsl_spline_init(spline, averageXPosList, averageHeightList,
        NUMBER_OF_AVERAGE_POINTS);

    enum { SEARCHING, drawing };
    int state = SEARCHING;

    int foundDrawPosition;
    for (int i = 0; i < fallenItemWidth; ++i) {
        int nextValue = gsl_spline_eval(spline, i, accelerator);

        switch (state) {
            case SEARCHING:
                if (nextValue != 0) {
                    foundDrawPosition = i;
                    cairo_move_to(cr, i, fallenItemHeight);
                    cairo_line_to(cr, i, fallenItemHeight);
                    cairo_line_to(cr, i, fallenItemHeight - nextValue);
                    state = drawing;
                }
                break;

            case drawing:
                cairo_line_to(cr, i, fallenItemHeight - nextValue);
                if (nextValue == 0 || i == fallenItemWidth - 1) {
                    cairo_line_to(cr, i, fallenItemHeight);
                    cairo_line_to(cr, foundDrawPosition, fallenItemHeight);
                    cairo_close_path(cr);
                    cairo_stroke_preserve(cr);
                    cairo_fill(cr);
                    state = SEARCHING;
                }
                break;
        }
    }

    gsl_spline_free(spline);
    gsl_interp_accel_free(accelerator);
    free(averageXPosList);
    free(averageHeightList);
    cairo_destroy(cr);
}

/** *********************************************************************
 ** This method ... threads: locking by caller
 **/
void eraseFallenAtPixel(FallenItem *fallen, int x) {
    if (fallen->fallenHeight[x] <= 0) {
        return;
    }

    sanelyCheckAndClearDisplayArea(mGlobal.display, mGlobal.StormWindow,
        fallen->x + x, fallen->y - fallen->fallenHeight[x], 1, 1, false);
    fallen->fallenHeight[x]--;
}

/** *********************************************************************
 ** This method ...
 **/
void eraseFallenOnDisplay(FallenItem *fallen, int xstart, int w) {
    int x = fallen->prevx;
    int y = fallen->prevy;

    sanelyCheckAndClearDisplayArea(mGlobal.display, mGlobal.StormWindow,
        x + xstart, y, w, fallen->h + mGlobal.MaxStormItemHeight, false);
}

/** *********************************************************************
 ** This method ...
 **/
void freeFallenItemMemory(FallenItem *fallen) {
    free(fallen->columnColor);
    free(fallen->fallenHeight);
    free(fallen->maxFallenHeight);

    cairo_surface_destroy(fallen->surface);
    cairo_surface_destroy(fallen->surface1);

    free(fallen);
}

/** *********************************************************************
 ** This method ...
 **/
FallenItem* findFallenItemByWindow(FallenItem* first,
    Window window) {
    FallenItem* fallen = first;

    while (fallen) {
        if (fallen->winInfo.window == window) {
            return fallen;
        }
        fallen = fallen->next;
    }

    return NULL;
}

/** *********************************************************************
 ** This method ...
 **/
void drawFallenItem(FallenItem* fallen) {
    if (fallen->winInfo.window == 0 ||
        (!fallen->winInfo.hidden &&
            (isFallenOnVisibleWorkspace(fallen) ||
                fallen->winInfo.sticky))) {

        createFallenDisplayArea(fallen);
    }
}

/** *********************************************************************
 ** This method ...
 **/
void cairoDrawAllFallenItems(cairo_t *cr) {
    if (!WorkspaceActive() || !Flags.ShowStormItems) {
        return;
    }
    if (!Flags.KeepFallenOnWindows && !Flags.KeepFallenOnDesktop) {
        return;
    }

    lockFallenSwapSemaphore();

    FallenItem *fallen = mGlobal.FallenFirst;
    while (fallen) {
        if (canFallenConsumeStormItem(fallen)) {
            cairo_set_source_surface(cr, fallen->surface,
                fallen->x, fallen->y - fallen->h);
            paintCairoContextWithAlpha(cr,
                (0.01 * (100 - Flags.Transparency)));

            fallen->prevx = fallen->x;
            fallen->prevy = fallen->y - fallen->h + 1;

            fallen->prevw = cairo_image_surface_get_width(
                fallen->surface);
            fallen->prevh = fallen->h;
        }

        fallen = fallen->next;
    }

    unlockFallenSwapSemaphore();
}

/** *********************************************************************
 ** This method ...
 **/
void swapFallenListItemSurfaces() {
    lockFallenSwapSemaphore();

    FallenItem* fallen = mGlobal.FallenFirst;
    while (fallen) {
        cairo_surface_t* tempSurface = fallen->surface1;
        fallen->surface1 = fallen->surface;
        fallen->surface = tempSurface;

        fallen = fallen->next;
    }

    unlockFallenSwapSemaphore();
}

/** *********************************************************************
 ** This method ...
 **/
// clean area for fallen with id
void eraseFallenListItem(Window window) {
    FallenItem* fallen = mGlobal.FallenFirst;
    while (fallen) {
        if (fallen->winInfo.window == window) {
            eraseFallenOnDisplay(fallen, 0, fallen->w);
            break;
        }
        fallen = fallen->next;
    }
}

/** *********************************************************************
 ** This method ...
 **/
// remove by id
int removeFallenListItem(FallenItem **list, Window id) {
    if (*list == NULL) {
        return 0;
    }

    // Do we hit on first item in List?
    FallenItem *fallen = *list;
    if (fallen->winInfo.window == id) {
        fallen = fallen->next;
        freeFallenItemMemory(*list);
        *list = fallen; // ?
        return 1;
    }

    // Loop through remainder of list items.
    FallenItem* scratch = NULL;
    while (true) {
        if (fallen->next == NULL) {
            return 0;
        }

        scratch = fallen->next;
        if (scratch->winInfo.window == id) {
            break;
        }

        fallen = fallen->next;
    }

    fallen->next = scratch->next;
    freeFallenItemMemory(scratch);

    return 1;
}


/** *********************************************************************
 ** This method ...
 **/
// print list
void logAllFallenDisplayAreas(FallenItem *list) {
    FallenItem *fallen = list;

    while (fallen != NULL) {
        int sumact = 0;

        for (int i = 0; i < fallen->w; i++) {
            sumact += fallen->fallenHeight[i];
        }
        printf("id:%#10lx ws:%4ld x:%6d y:%6d w:%6d sty:%2d hid:%2d sum:%8d\n",
            fallen->winInfo.window, fallen->winInfo.ws, fallen->x, fallen->y, fallen->w,
            fallen->winInfo.sticky, fallen->winInfo.hidden, sumact);
        fallen = fallen->next;
    }
}
