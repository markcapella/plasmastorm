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

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "Blowoff.h"
#include "ClockHelper.h"
#include "Fallen.h"
#include "hashTableHelper.h"
#include "MainWindow.h"
#include "pixmaps.h"
#include "plasmastorm.h"
#include "Prefs.h"
#include "safeMalloc.h"
#include "Storm.h"
#include "utils.h"
#include "Wind.h"
#include "windows.h"
#include "xpmHelper.h"


/***********************************************************
 * Externally provided to this Module.
 */

// Color Picker methods.
bool isQPickerActive();
char* getQPickerCallerName();
bool isQPickerVisible();
bool isQPickerTerminated();

int getQPickerRed();
int getQPickerBlue();
int getQPickerGreen();

void endQPickerDialog();


/** *********************************************************************
 ** Module globals and consts.
 **/
#define RANDOM_STORMITEM_COUNT 300

#define STORMITEMS_PERSEC_PERPIXEL 30

#define MAX_WIND_SENSITIVITY 0.4
#define INITIAL_Y_SPEED 120

const float mStormItemSpeedAdjustment = 0.7;
const float mStormItemSizeAdjustment = 0.8;

const float mWindSpeedMaxArray[] =
    { 100.0, 300.0, 600.0, };

static int mPreviousAppScale = 100;

int mResourcesShapeCount = 0;

static int mStormItemsShapeCount = 0;
static char*** mStormItemShapes = NULL;


static StormItemSurface* mStormItemSurfaceList = NULL;

bool mCreateEventInitialized = false;
static double mCreateStormItemsStartTimePrevious;
static double mCreateEventStartedDesiringTime;

bool mStallingNewStormItemCreateEvents = false;

static float mStormItemsPerSecond;
static float mStormItemsSpeedFactor;

int mStormItemColorToggle = 0;
GdkRGBA mStormItemColor;


/** *********************************************************************
 ** This method initializes the storm module.
 **/
void initStormModule() {
    // Create ShapesList from Resources & new Random Storm Shapes.
    mResourcesShapeCount = getResourcesShapeCount();

    createCombinedShapesList();
    createCombinedShapeSurfacesList();
    updateStormShapesAttributes();

    setStormItemSpeed();
    setStormItemsPerSecond();

    addMethodToMainloop(PRIORITY_DEFAULT, DO_CREATE_STORMITEM_EVENT,
        doCreateStormShapeEvent);
}

/** *********************************************************************
 ** Helper returns count of pixmaps @ runtime of Shapes
 ** loaded via generated includes @ compile time.
 **/
int getResourcesShapeCount() {
    int count = 0;
    while (mResourcesShapes[count]) {
        count++;
    }

    return count;
}

/** *********************************************************************
 ** This method loads and/or creates stormItem pixmap images.
 **/
void createCombinedShapesList() {
    // Remove any existing Shape list.
    if (mStormItemShapes) {
        for (int i = 0; i < mStormItemsShapeCount; i++) {
            xpm_destroy(mStormItemShapes[i]);
        }
        free(mStormItemShapes);
    }

    // Create new combined Shape list.
    mStormItemsShapeCount = mResourcesShapeCount +
        RANDOM_STORMITEM_COUNT;

    char*** newShapesList = (char***) malloc(
        mStormItemsShapeCount * sizeof(char**));

    // First, add Shapes from resources Shapes list.
    int lineCount;
    for (int i = 0; i < mResourcesShapeCount; i++) {
        xpm_set_color((char**) mResourcesShapes[i],
            &newShapesList[i], &lineCount, "storm");
    }

    // Then Shapes generated randomly.
    for (int i = 0; i < RANDOM_STORMITEM_COUNT; i++) {
        createRandomStormShape(
            Flags.ShapeSizeFactor + (Flags.ShapeSizeFactor * drand48()),
            Flags.ShapeSizeFactor + (Flags.ShapeSizeFactor * drand48()),
            &newShapesList[i + mResourcesShapeCount]);
    }

    mStormItemShapes = newShapesList;
}

/** *********************************************************************
 ** This method initializes a ShapeSurface array for each Resource
 ** shape & Random shape in the StormItem Shapes Array.
 **/
void createCombinedShapeSurfacesList() {
    mStormItemSurfaceList = (StormItemSurface*) malloc(
        mStormItemsShapeCount * sizeof(StormItemSurface));

    for (int i = 0; i < mStormItemsShapeCount; i++) {
        mStormItemSurfaceList[i].surface = NULL;
    }
}

/** *********************************************************************
 ** This method updates stormItem pixmap image attributes such as color.
 **/
void updateStormShapesAttributes() {
    for (int i = 0; i < mStormItemsShapeCount; i++) {
        // Reset each Shape base to StormColor.
        char** shapeString;
        int lineCount;
        xpm_set_color(mStormItemShapes[i], &shapeString, &lineCount,
            getNextStormShapeColorAsString());

        // Get item base w/h, and rando shrink item size w/h.
        // Items stuck on scenery removes themself this way.
        int itemWidth, itemHeight;
        sscanf(mStormItemShapes[i][0], "%d %d",
            &itemWidth, &itemHeight);
        itemWidth *= 0.01 * Flags.Scale *
            mStormItemSizeAdjustment * mGlobal.WindowScale;
        itemHeight *= 0.01 * Flags.Scale *
            mStormItemSizeAdjustment * mGlobal.WindowScale;

        // Set new item draw surface.
        // Guard stormItem w/h.
        StormItemSurface* itemSurface =
            &mStormItemSurfaceList[i];
        if (itemWidth < 1) {
            itemWidth = 1;
        }
        if (itemHeight < 1) {
            itemHeight = 1;
        }
        if (itemWidth == 1 && itemHeight == 1) {
            itemHeight = 2;
        }
        itemSurface->width = itemWidth;
        itemSurface->height = itemHeight;

        // Destroy existing surface.
        if (itemSurface->surface) {
            cairo_surface_destroy(itemSurface->surface);
        }

        // Create new surface from base.
        GdkPixbuf* newStormItemSurface =
            gdk_pixbuf_new_from_xpm_data((const char**) shapeString);
        xpm_destroy(shapeString);

        // Scale it to new size.
        GdkPixbuf* pixbufscaled = gdk_pixbuf_scale_simple(
            newStormItemSurface, itemWidth, itemHeight,
            GDK_INTERP_HYPER);
        itemSurface->surface = gdk_cairo_surface_create_from_pixbuf(
            pixbufscaled, 0, NULL);
        g_clear_object(&pixbufscaled);

        g_clear_object(&newStormItemSurface);
    }
}

/** *********************************************************************
 ** This method updates module based on User pref settings.
 **/
void respondToStormsSettingsChanges() {
    //UIDO(ShowStormItems,
    if (Flags.ShowStormItems !=
            OldFlags.ShowStormItems) {
        OldFlags.ShowStormItems =
            Flags.ShowStormItems;
        if (!Flags.ShowStormItems) {
            clearStormWindow();
        }
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(ShapeSizeFactor, 
    if (Flags.ShapeSizeFactor !=
            OldFlags.ShapeSizeFactor) {
        OldFlags.ShapeSizeFactor =
            Flags.ShapeSizeFactor;
        setStormItemSize();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(StormItemSpeedFactor,
    if (Flags.StormItemSpeedFactor !=
            OldFlags.StormItemSpeedFactor) {
        OldFlags.StormItemSpeedFactor =
            Flags.StormItemSpeedFactor;
        setStormItemSpeed();
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(StormItemCountMax, );
    if (Flags.StormItemCountMax !=
            OldFlags.StormItemCountMax) {
        OldFlags.StormItemCountMax =
            Flags.StormItemCountMax;
        Flags.mHaveFlagsChanged++;
    }

    //UIDO(StormSaturationFactor,
    if (Flags.StormSaturationFactor !=
            OldFlags.StormSaturationFactor) {
        OldFlags.StormSaturationFactor =
            Flags.StormSaturationFactor;
        setStormItemsPerSecond();
        Flags.mHaveFlagsChanged++;
    }

    //UIDOS(StormItemColor1, updateStormShapesAttributes();
    //    clearStormWindow(););
    if (strcmp(Flags.StormItemColor1, OldFlags.StormItemColor1)) {
        updateStormShapesAttributes();
        clearStormWindow();
        free(OldFlags.StormItemColor1);
        OldFlags.StormItemColor1 = strdup(Flags.StormItemColor1);
        Flags.mHaveFlagsChanged++;
    }

    if (isQPickerActive() && !strcmp(getQPickerCallerName(),
        "StormItemColor1TAG") && !isQPickerVisible()) {
        char cbuffer[16];
        snprintf(cbuffer, 16, "#%02x%02x%02x", getQPickerRed(),
            getQPickerGreen(), getQPickerBlue());

        GdkRGBA color;
        gdk_rgba_parse(&color, cbuffer);
        free(Flags.StormItemColor1);
        rgba2color(&color, &Flags.StormItemColor1);

        endQPickerDialog();
    }

    //UIDOS(StormItemColor2, updateStormShapesAttributes();
    // clearStormWindow(););
    if (strcmp(Flags.StormItemColor2, OldFlags.StormItemColor2)) {
        updateStormShapesAttributes();
        clearStormWindow();
        free(OldFlags.StormItemColor2);
        OldFlags.StormItemColor2 = strdup(Flags.StormItemColor2);
        Flags.mHaveFlagsChanged++;
    }

    if (isQPickerActive() && !strcmp(getQPickerCallerName(),
        "StormItemColor2TAG") && !isQPickerVisible()) {
        char cbuffer[16];
        snprintf(cbuffer, 16, "#%02x%02x%02x", getQPickerRed(),
            getQPickerGreen(), getQPickerBlue());

        GdkRGBA color;
        gdk_rgba_parse(&color, cbuffer);
        free(Flags.StormItemColor2);
        rgba2color(&color, &Flags.StormItemColor2);

        endQPickerDialog();
    }

    if (hasAppScaleChangedFrom(&mPreviousAppScale)) {
        updateStormShapesAttributes();
    }
}

/** *********************************************************************
 ** This method sets the desired size of the StormItem.
 **/
void setStormItemSize() {
    createCombinedShapesList();
    updateStormShapesAttributes();

    clearStormWindow();
}

/** *********************************************************************
 ** This method sets the desired speed of the StormItem.
 **/
void setStormItemSpeed() {
    if (Flags.StormItemSpeedFactor < 10) {
        mStormItemsSpeedFactor = 0.01 * 10;
    } else {
        mStormItemsSpeedFactor = 0.01 * Flags.StormItemSpeedFactor;
    }
    mStormItemsSpeedFactor *= mStormItemSpeedAdjustment;
}

/** *********************************************************************
 ** This method sets the StormItem "fluff" state.
 **/
void setStormItemState(StormItem *stormItem, float t) {
    // Early exit if already fluffing.
    if (stormItem->fluff) {
        return;
    }

    // Fluff it.
    stormItem->fluff = 1;
    stormItem->flufftimer = 0;
    stormItem->flufftime = (t > 0.01) ? t : 0.01;

    mGlobal.FluffedStormItemCount++;
}

/** *********************************************************************
 ** This method sets a Items-Per-Second throttle rate.
 **/
void setStormItemsPerSecond() {
    mStormItemsPerSecond = mGlobal.StormWindowWidth * 0.0015 *
        Flags.StormSaturationFactor * 0.001 * STORMITEMS_PERSEC_PERPIXEL *
        mStormItemsSpeedFactor;
}

/** *********************************************************************
 ** This method adds a batch of items to the Storm window.
 **/
int doCreateStormShapeEvent() {
    if (Flags.shutdownRequested) {
        return false;
    }

    const int eventStartTime = wallclock();
    if (mStallingNewStormItemCreateEvents) {
        mCreateStormItemsStartTimePrevious = eventStartTime;
        return true;
    }
    if ((!WorkspaceActive() || !Flags.ShowStormItems)) {
        mCreateStormItemsStartTimePrevious = eventStartTime;
        return true;
    }

    // Initialize method globals.
    if (!mCreateEventInitialized) {
        mCreateStormItemsStartTimePrevious = eventStartTime;
        mCreateEventStartedDesiringTime = 0;
        mCreateEventInitialized = true;
    }

    // After suspend or sleep, eventElapsedTime
    // could have a strange value.
    const double eventElapsedTime = eventStartTime -
        mCreateStormItemsStartTimePrevious;
    if (eventElapsedTime < 0 ||
        eventElapsedTime > 10 * DO_CREATE_STORMITEM_EVENT) {
        mCreateStormItemsStartTimePrevious = eventStartTime;
        return true;
    }

    // Determine how many items we can create.
    const int desiredItemsCount = lrint((eventElapsedTime +
        mCreateEventStartedDesiringTime) * mStormItemsPerSecond);

    // Early out if none desired due to busy.
    if (desiredItemsCount == 0) {
        mCreateEventStartedDesiringTime += eventElapsedTime;
        mCreateStormItemsStartTimePrevious = eventStartTime;
        return true;
    }

    // Create the items.
    for (int i = 0; i < desiredItemsCount; i++) {
        createStormItem(Flags.ComboStormShape - 1);
    }

    // Update method global & done.
    mCreateStormItemsStartTimePrevious = eventStartTime;
    mCreateEventStartedDesiringTime = 0;
    return true;
}

/** *********************************************************************
 ** This method stalls new StormItem creation events.
 **/
int doStallCreateStormShapeEvent() {
    if (Flags.shutdownRequested) {
        return false;
    }

    // Kill all items until none exist.
    if (mGlobal.StormItemCount > 0) {
        mStallingNewStormItemCreateEvents = true;
        return true;
    }

    mStallingNewStormItemCreateEvents = false;
    return false;
}

/** *********************************************************************
 ** This method creates a item from itemType (or random).
 **
 ** The stormItem is initialized, and inserted into the main array
 ** (hashtable set).
 **/
StormItem* createStormItem(int itemType) {
    mGlobal.StormItemCount++;

    // If itemType < 0, create random itemType.
    if (itemType < 0) {
        itemType = mResourcesShapeCount + drand48() *
            (mStormItemsShapeCount - mResourcesShapeCount);
    }

    StormItem* stormItem = (StormItem*) malloc(
        sizeof(StormItem));
    stormItem->shapeType = itemType;
    pushStormItemIntoItemset(stormItem);

    addMethodWithArgToMainloop(PRIORITY_HIGH,
        DO_STORMITEM_UPDATE_EVENT_TIME,
        (GSourceFunc) updateStormItem, stormItem);

    return stormItem;
}

/** *********************************************************************
 ** This method updates a stormItem object.
 **/
int updateStormItem(StormItem* stormItem) {
    if (!WorkspaceActive() || !Flags.ShowStormItems) {
        return true;
    }

    // Candidate for removal?
    if (mGlobal.RemoveFluff) {
        if (stormItem->fluff || stormItem->isFrozen) {
            eraseStormItem(stormItem);
            removeStormItemInItemset(stormItem);
            return false;
        }
    }

    // Candidate for removal?
    if (mStallingNewStormItemCreateEvents) {
        eraseStormItem(stormItem);
        removeStormItemInItemset(stormItem);
        return false;
    }

    // Candidate for removal?
    if (stormItem->fluff &&
        stormItem->flufftimer > stormItem->flufftime) {
        eraseStormItem(stormItem);
        removeStormItemInItemset(stormItem);
        return false;
    }

    // StormItem X /Y screen positions.
    const double stormItemUpdateTime =
        DO_STORMITEM_UPDATE_EVENT_TIME;

    float NewX = stormItem->xRealPosition +
        (stormItem->xVelocity * stormItemUpdateTime) *
        mStormItemsSpeedFactor;
    float NewY = stormItem->yRealPosition +
        (stormItem->yVelocity * stormItemUpdateTime) *
        mStormItemsSpeedFactor;

    if (stormItem->fluff) {
        if (!stormItem->isFrozen) {
            stormItem->xRealPosition = NewX;
            stormItem->yRealPosition = NewY;
        }
        stormItem->flufftimer += stormItemUpdateTime;
        return true;
    }

    // Low probability to remove each, High probability
    // To remove blown-off.
    const bool itemsRequireRemoval = (mGlobal.StormItemCount -
        mGlobal.FluffedStormItemCount) >= Flags.StormItemCountMax;

    if (itemsRequireRemoval) {
        if ((!stormItem->cyclic && (drand48() > 0.3)) ||
            (drand48() > 0.9)) {
            setStormItemState(stormItem, 0.51);
            return true;
        }
    }

    // Update speed in x Direction if wind blowing.
    if (Flags.ShowWind) {
        float force = stormItemUpdateTime *
            stormItem->windSensitivity / stormItem->massValue;
        if (force > 0.9) {
            force = 0.9;
        }
        if (force < -0.9) {
            force = -0.9;
        }
        stormItem->xVelocity += force *
            (mGlobal.NewWind - stormItem->xVelocity);

        const float X_DIR_SPEED_BOUND =
            mWindSpeedMaxArray[mGlobal.Wind] * 2;
        if (fabs(stormItem->xVelocity) > X_DIR_SPEED_BOUND) {
            if (stormItem->xVelocity > X_DIR_SPEED_BOUND) {
                stormItem->xVelocity = X_DIR_SPEED_BOUND;
            }
            if (stormItem->xVelocity < -X_DIR_SPEED_BOUND) {
                stormItem->xVelocity = -X_DIR_SPEED_BOUND;
            }
        }
    }

    // Update speed in y Direction.
    stormItem->yVelocity += INITIAL_Y_SPEED * (drand48() - 0.4) * 0.1;
    if (stormItem->yVelocity > stormItem->initialYVelocity * 1.5) {
        stormItem->yVelocity = stormItem->initialYVelocity * 1.5;
    }

    // If stormItem frozen to something, all done here.
    if (stormItem->isFrozen) {
        return true;
    }

    // Update the Cyclic items position.
    if (stormItem->cyclic) {
        if (NewX < -mStormItemSurfaceList[stormItem->shapeType].width) {
            NewX += mGlobal.StormWindowWidth - 1;
        }
        if (NewX >= mGlobal.StormWindowWidth) {
            NewX -= mGlobal.StormWindowWidth;
        }
    }

    // NonCyclic items die when going left or right
    // out of the window.
    if (!stormItem->cyclic) {
        if (NewX < 0 || NewX >= mGlobal.StormWindowWidth) {
            removeStormItemInItemset(stormItem);
            return false;
        }
    }

    // remove stormItem if it falls below bottom of screen:
    if (NewY >= mGlobal.StormWindowHeight) {
        removeStormItemInItemset(stormItem);
        return false;
    }

    // Fallen interaction.
    if (!stormItem->fluff) {
        lockFallenSemaphore();
        if (isStormItemFallen(stormItem,
            lrintf(NewX), lrintf(NewY))) {
            removeStormItemInItemset(stormItem);
            unlockFallenSemaphore();
            return false;
        }
        unlockFallenSemaphore();

        stormItem->isVisible = isStormItemBehindWindow(stormItem,
            lrintf(NewX), lrintf(NewY));
    }

    stormItem->xRealPosition = NewX;
    stormItem->yRealPosition = NewY;
    return true;
}

/** *********************************************************************
 ** This method generated a random xpm pixmap
 ** with dimensions xpmWidth x xpmHeight.
 **
 ** The stormItem will be rotated, so the w and h of the resulting
 ** xpm will be different from the input w and h.
 **/
void createRandomStormShape(int xpmWidth, int xpmHeight, char*** xpm) {
    // Initialize with @ least one pixel in the middle.
    const int xpmArrayLength = xpmWidth * xpmHeight;

    float* itemXArray = (float*)
        malloc(xpmArrayLength * sizeof(float));
    float* itemYArray = (float*)
        malloc(xpmArrayLength * sizeof(float));

    int itemArrayLength = 1;
    itemXArray[0] = 0;
    itemYArray[0] = 0;

    // Pre-calc for faster loop.
    const float halfXpmWidth = 0.5 * xpmWidth;
    const float halfXpmHeight = 0.5 * xpmHeight;

    // Loop.
    for (int xpmHeightIndex = 0; xpmHeightIndex < xpmHeight;
        xpmHeightIndex++) {
        const float rotateHeight = (xpmHeightIndex > halfXpmHeight) ?
            xpmHeight - xpmHeightIndex : xpmHeightIndex;
        const float py = 2 * rotateHeight / xpmHeight;

        for (int xpmWidthIndex = 0; xpmWidthIndex < xpmWidth;
            xpmWidthIndex++) {
            const float rotateWidth = (xpmWidthIndex > halfXpmWidth) ?
                xpmWidth - xpmWidthIndex : xpmWidthIndex;
            const float px = 2 * rotateWidth / xpmWidth;

            // Push arrayItem on eventProbability.
            const float eventProbability = 1.1 - (px * py);
            if (drand48() > eventProbability) {
                if (itemArrayLength < xpmArrayLength) {
                    itemYArray[itemArrayLength] = xpmHeightIndex - halfXpmWidth;
                    itemXArray[itemArrayLength] = xpmWidthIndex - halfXpmHeight;
                    itemArrayLength++;
                }
            }
        }
    }

    // Rotate points with a random angle 0 .. pi. (Rick Magic?)
    const float randomAngle = drand48() * M_PI;
    const float cosRandomAngle = cosf(randomAngle);
    const float sinRandomAngle = sinf(randomAngle);

    // Init array & null terminate.
    float* rotatedXArray = (float*)
        malloc(itemArrayLength * sizeof(float));
    float* rotatedYArray = (float*)
        malloc(itemArrayLength * sizeof(float));
    rotatedXArray[0] = 0;
    rotatedYArray[0] = 0;

    // Copy array and rotate.
    for (int i = 0; i < itemArrayLength; i++) {
        rotatedXArray[i] = itemXArray[i] * cosRandomAngle -
            itemYArray[i] * sinRandomAngle;
        rotatedYArray[i] = itemXArray[i] * sinRandomAngle +
            itemYArray[i] * cosRandomAngle;
    }

    // Find min height and width of rotated XPM Image.
    float xmin = rotatedXArray[0];
    float xmax = rotatedXArray[0];
    float ymin = rotatedYArray[0];
    float ymax = rotatedYArray[0];

    for (int i = 0; i < itemArrayLength; i++) {
        if (rotatedXArray[i] < xmin) {
            xmin = rotatedXArray[i];
        }
        if (rotatedXArray[i] > xmax) {
            xmax = rotatedXArray[i];
        }
        if (rotatedYArray[i] < ymin) {
            ymin = rotatedYArray[i];
        }
        if (rotatedYArray[i] > ymax) {
            ymax = rotatedYArray[i];
        }
    }

    // Expand 1x1 image to 1x2.
    int nw = ceilf(xmax - xmin + 1);
    int nh = ceilf(ymax - ymin + 1);
    if (nw == 1 && nh == 1) {
        nh = 2;
    }

    // Rick(?) : If min height and width are 1,
    // then we have trouble.
    assert(nh > 0);
    assert(nw >= 0);

    // Create XPM commmand strings.
    const int xpmNumberOfStringArrayHeaderEntries = 3;

    *xpm = (char**) malloc((nh + xpmNumberOfStringArrayHeaderEntries) *
        sizeof(char*));
    char** xpmStringArray = *xpm;

    xpmStringArray[0] = (char*) malloc(80 * sizeof(char));
    snprintf(xpmStringArray[0], 80, "%d %d 2 1", nw, nh);

    xpmStringArray[1] = strdup("  c None");

    xpmStringArray[2] = (char*) malloc(80 * sizeof(char));
    const char periodCharacter = '.';
    snprintf(xpmStringArray[2], 80, "%c c black",
        periodCharacter);

    for (int i = 0; i < nh; i++) {
        xpmStringArray[i + xpmNumberOfStringArrayHeaderEntries] =
            (char*) malloc((nw + 1) * sizeof(char));
    }

    for (int i = 0; i < nh; i++) {
        for (int j = 0; j < nw; j++) {
            xpmStringArray[i + xpmNumberOfStringArrayHeaderEntries] [j] = ' ';
        }
        xpmStringArray[i + xpmNumberOfStringArrayHeaderEntries] [nw] = 0;
    }

    for (int i = 0; i < itemArrayLength; i++) {
        const int fya = (int) rotatedYArray[i] - ymin +
            xpmNumberOfStringArrayHeaderEntries;
        const int fxa = (int) rotatedXArray[i] - xmin;

        xpmStringArray[fya][fxa] = periodCharacter;
    }

    free(itemXArray);
    free(itemYArray);

    free(rotatedXArray);
    free(rotatedYArray);
}

/** *********************************************************************
 ** This method erases a single stormItem pixmap from the display.
 **/
void eraseStormItem(StormItem* stormItem) {
    const int xPos = stormItem->xIntPosition - 1;
    const int yPos = stormItem->yIntPosition - 1;

    const int itemWidth = 2 +
        mStormItemSurfaceList[stormItem->shapeType].width;
    const int itemHeight = 2 +
        mStormItemSurfaceList[stormItem->shapeType].height;

    sanelyCheckAndClearDisplayArea(mGlobal.display, mGlobal.StormWindow,
        xPos, yPos, itemWidth, itemHeight, false);
}

/** *********************************************************************
 ** These are helper methods for ItemColor.
 **/
void setStormShapeColor(GdkRGBA itemColor) {
    mStormItemColor = itemColor;
}

/** *********************************************************************
 ** These are helper methods for ItemColor.
 **/
char* getNextStormShapeColorAsString() {
    // Toggle color switch.
    mStormItemColorToggle = (mStormItemColorToggle == 0) ? 1 : 0;

    // Update color.
    GdkRGBA nextColor;
    if (mStormItemColorToggle == 0) {
        gdk_rgba_parse(&nextColor, Flags.StormItemColor1);
    } else {
        gdk_rgba_parse(&nextColor, Flags.StormItemColor2);
    }
    setStormShapeColor(nextColor);

    // Return result as string.
    return (mStormItemColorToggle == 0) ?
        Flags.StormItemColor1 : Flags.StormItemColor2;
}

/** *********************************************************************
 ** These are helper methods for ItemColor.
 **/
GdkRGBA getNextStormShapeColorAsRGB() {
    // Toggle color switch.
    mStormItemColorToggle = (mStormItemColorToggle == 0) ? 1 : 0;

    // Update color.
    GdkRGBA nextColor;
    if (mStormItemColorToggle == 0) {
        gdk_rgba_parse(&nextColor, Flags.StormItemColor1);
    } else {
        gdk_rgba_parse(&nextColor, Flags.StormItemColor2);
    }
    setStormShapeColor(nextColor);

    return nextColor;
}

/** *********************************************************************
 ** This method checks if stormItem is in a not-hidden
 ** fallen on current workspace.
 **
 ** The bottom pixels of the item are at:
 ** The bottompixels are at:
 **      x = NewX .. NewX + width-of-stormItem - 1
 **      y = NewY + (height of stormItem)
 **/
bool isStormItemFallen(StormItem* stormItem,
    int xPos, int yPos) {

    const int itemWidth =
        mStormItemSurfaceList[stormItem->shapeType].width;

    FallenItem* fallen = mGlobal.FallenFirst;
    while (fallen) {
        if (fallen->winInfo.hidden) {
            fallen = fallen->next;
            continue;
        }

        if (fallen->winInfo.window != None &&
            !isFallenOnVisibleWorkspace(fallen) &&
            !fallen->winInfo.sticky) {
            fallen = fallen->next;
            continue;
        }

        if (xPos < fallen->x ||
            xPos > fallen->x + fallen->w ||
            yPos >= fallen->y + 2) {
            fallen = fallen->next;
            continue;
        }

        // StormItem hits first FallenItem & we're done.
        int istart = xPos - fallen->x;
        if (istart < 0) {
            istart = 0;
        }
        int imax = istart + itemWidth;
        if (imax > fallen->w) {
            imax = fallen->w;
        }

        for (int i = istart; i < imax; i++) {
            if (yPos > fallen->y - fallen->fallenHeight[i] - 1) {
                if (fallen->fallenHeight[i] < fallen->maxFallenHeight[i]) {
                    updateFallenPartial(fallen, xPos - fallen->x,
                        itemWidth);
                }

                if (canFallenConsumeStormItem(fallen)) {
                    setStormItemState(stormItem, .9);
                    if (!stormItem->fluff) {
                        return true;
                    }
                }

                return false;
            }
        }

        // Otherwise, loop thru all.
        fallen = fallen->next;
    }

    return false;
}

/** *********************************************************************
 ** This method checks if stormItem is behind a window.
 **/
bool isStormItemBehindWindow(StormItem* stormItem,
        int xPos, int yPos) {

    return isAreaClippedByWindow(xPos, yPos,
        mStormItemSurfaceList[stormItem->shapeType].width,
        mStormItemSurfaceList[stormItem->shapeType].height);
}

/** *********************************************************************
 ** Itemset hashtable helper - Push a new item into the list.
 **/
void pushStormItemIntoItemset(StormItem* stormItem) {
    stormItem->color = mStormItemColor;

    stormItem->cyclic = 1;
    stormItem->isFrozen = false;
    stormItem->isVisible = true;

    stormItem->fluff = 0;
    stormItem->flufftimer = 0;
    stormItem->flufftime = 0;

    const int itemWidth =
        mStormItemSurfaceList[stormItem->shapeType].width;
    stormItem->xRealPosition =
        randint(mGlobal.StormWindowWidth - itemWidth);

    const int itemHeight =
        mStormItemSurfaceList[stormItem->shapeType].height;
    stormItem->yRealPosition =
        -randint(mGlobal.StormWindowHeight / 10) - itemHeight;

    stormItem->massValue = drand48() + 0.1;

    stormItem->windSensitivity = drand48() *
        MAX_WIND_SENSITIVITY;

    stormItem->initialYVelocity = INITIAL_Y_SPEED *
        sqrt(stormItem->massValue);

    stormItem->xVelocity = (Flags.ShowWind) ?
        randint(mGlobal.NewWind) / 2 : 0;
    stormItem->yVelocity = stormItem->initialYVelocity;

    set_insert(stormItem);
}

/** *********************************************************************
 ** Itemset hashtable helper - Draw all items on the display.
 **/
int drawAllStormItemsInItemset(cairo_t* cr) {
    if (!Flags.ShowStormItems) {
        return true;
    }

    set_begin();

    StormItem* stormItem;
    while ((stormItem = (StormItem*) set_next())) {
        cairo_set_source_surface(cr,
            mStormItemSurfaceList[stormItem->shapeType].surface,
            stormItem->xRealPosition, stormItem->yRealPosition);

        // Determine stormItem alpha from base user setting.
        double alpha = (0.01 * (100 - Flags.Transparency));

        // Fluff across time, and guard the lower bound.
        if (stormItem->fluff) {
            alpha *= (1 - stormItem->flufftimer / stormItem->flufftime);
        }
        if (alpha < 0) {
            alpha = 0;
        }

        // Draw.
        if (!stormItem->isFrozen && !stormItem->fluff) {
            my_cairo_paint_with_alpha(cr, alpha);
        }

        // Update 
        stormItem->xIntPosition = lrint(stormItem->xRealPosition);
        stormItem->yIntPosition = lrint(stormItem->yRealPosition);
    }

    return true;
}

/** *********************************************************************
 ** Itemset hashtable helper - Remove all items from the list.
 **/
int removeAllStormItemsInItemset() {
    set_begin();

    StormItem* stormItem;
    while ((stormItem = (StormItem*) set_next())) {
        eraseStormItem(stormItem);
    }

    return true;
}

/** *********************************************************************
 ** Itemset hashtable helper - Remove a specific item from the list.
 **
 ** Calls to this method from the mainloop() *MUST* be followed by
 ** a 'return false;' to remove this stormItem from the g_timeout callback.
 **/
void removeStormItemInItemset(StormItem* stormItem) {
    if (stormItem->fluff) {
        mGlobal.FluffedStormItemCount--;
    }

    set_erase(stormItem);

    free(stormItem);
    mGlobal.StormItemCount--;
}

/** *********************************************************************
 ** This method is a debugging helper.
 **/
void logStormItem(StormItem* stormItem) {
    printf("stormItem: %p sens: %6.0f  "
        "x/y %6.0f %6.0f  xVelocity/yVelocity: %6.0f %6.0f  "
        "flf: %d  frz: %d  "
        "ftnow: %8.3f  ftmax: %8.3f\n",

        (void*) stormItem, stormItem->windSensitivity,
        stormItem->xRealPosition, stormItem->yRealPosition, stormItem->xVelocity, stormItem->yVelocity,
        stormItem->fluff, stormItem->isFrozen,
        stormItem->flufftimer, stormItem->flufftime);
}
