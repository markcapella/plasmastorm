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
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <gtk/gtk.h>

#include "pixmaps.h"
#include "Prefs.h"
#include "safeMalloc.h"
#include "Stars.h"
#include "utils.h"
#include "windows.h"


/** *********************************************************************
 ** Module globals and consts.
 **/

#define STARANIMATIONS 4

static const int STAR_SIZE = 9;
static const float LOCAL_SCALE = 0.8;

static int mNumberOfStars;

static StarCoordinate* mStarCoordinates = NULL;

static char* mStarColorArray[STARANIMATIONS] =
    { (char*) "gold", (char*) "gold1",
      (char*) "gold4", (char*) "orange"};

static cairo_surface_t* starScreenSurfaces[STARANIMATIONS];

static int mPreviousAppScale = 100;

/** *********************************************************************
 ** This method initializes the Stars module.
 **/
void initStarsModule() {
    initStarsModuleArrays();

    // Clear and set starScreenSurfaces.
    for (int i = 0; i < STARANIMATIONS; i++) {
        starScreenSurfaces[i] = NULL;
    }
    initStarsModuleSurfaces();

    addMethodToMainloop(PRIORITY_DEFAULT, time_ustar, updateStarsFrame);
}

/** *********************************************************************
 ** This method updates Stars module settings between
 ** Erase and Draw cycles.
 **/
void initStarsModuleArrays() {
    mNumberOfStars = Flags.MaxStarCount;

    mStarCoordinates = (StarCoordinate*) realloc(mStarCoordinates,
        (mNumberOfStars + 1) * sizeof(StarCoordinate));
    REALLOC_CHECK(mStarCoordinates);

    for (int i = 0; i < mNumberOfStars; i++) {
        StarCoordinate *star = &mStarCoordinates[i];
        star->x = randint(mGlobal.StormWindowWidth);
        star->y = randint(mGlobal.StormWindowHeight / 4);
        star->color = randint(STARANIMATIONS);
    }
}

/** *********************************************************************
 ** This method inits cairo surfaces.
 **/
void initStarsModuleSurfaces() {
    for (int i = 0; i < STARANIMATIONS; i++) {
        float size = LOCAL_SCALE * mGlobal.WindowScale *
            0.01 * Flags.Scale * STAR_SIZE;

        size *= 0.2 * (1 + 4 * drand48());
        if (size < 1) {
            size = 1;
        }

        // Release and recreate surfaces.
        if (starScreenSurfaces[i]) {
            cairo_surface_destroy(starScreenSurfaces[i]);
        }
        starScreenSurfaces[i] = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, size, size);

        cairo_t *cr = cairo_create(starScreenSurfaces[i]);
        cairo_set_line_width(cr, 1.0 * size / STAR_SIZE);

        GdkRGBA color;
        gdk_rgba_parse(&color, mStarColorArray[i]);

        cairo_set_source_rgba(cr, color.red,
            color.green, color.blue, color.alpha);

        cairo_move_to(cr, 0, 0);
        cairo_line_to(cr, size, size);
        cairo_move_to(cr, 0, size);
        cairo_line_to(cr, size, 0);
        cairo_move_to(cr, 0, size / 2);
        cairo_line_to(cr, size, size / 2);
        cairo_move_to(cr, size / 2, 0);
        cairo_line_to(cr, size / 2, size);
        cairo_stroke(cr);

        cairo_destroy(cr);
    }
}

/** *********************************************************************
 ** This method erases a single Stars
 ** frame from drawCairoWindowInternal().
 **/
void eraseStarsFrame() {
    if (!Flags.ShowStars) {
        return;
    }

    for (int i = 0; i < mNumberOfStars; i++) {
        StarCoordinate *star = &mStarCoordinates[i];

        sanelyCheckAndClearDisplayArea(mGlobal.display,
            mGlobal.StormWindow, star->x, star->y,
            STAR_SIZE, STAR_SIZE, false);
    }
}

/** *********************************************************************
 ** This method updates Stars module between
 ** Erase and Draw cycles.
 **/
int updateStarsFrame() {
    if (Flags.shutdownRequested) {
        return FALSE;
    }
    if (!WorkspaceActive()) {
        return TRUE;
    }

    for (int i = 0; i < mNumberOfStars; i++) {
        if (drand48() > 0.8) {
            mStarCoordinates[i].color = randint(STARANIMATIONS);
        }
    }

    return TRUE;
}

/** *********************************************************************
 ** This method draws a single Stars
 ** frame from drawCairoWindowInternal().
 **/
void drawStarsFrame(cairo_t *cr) {
    if (!Flags.ShowStars) {
        return;
    }

    cairo_save(cr);

    cairo_set_line_width(cr, 1);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    for (int i = 0; i < mNumberOfStars; i++) {
        StarCoordinate* star = &mStarCoordinates[i];
        cairo_set_source_surface(cr, starScreenSurfaces[star->color],
            star->x, star->y);

        if (isAreaClippedByWindow(star->x, star->y,
            STAR_SIZE, STAR_SIZE)) {
            my_cairo_paint_with_alpha(cr,
                (0.01 * (100 - Flags.Transparency)));
        }
    }

    cairo_restore(cr);
}

/** *********************************************************************
 ** This method updates the Stars module with
 ** refreshed user settings.
 **/
void updateStarsUserSettings() {
    //UIDO(Stars, 
    //printf("plasmastorm: updateStarsUserSettings() Starts for ShowStars: %i %i.\n",
    //    Flags.ShowStars, gtk_toggle_button_get_active(
    //        GTK_TOGGLE_BUTTON((GtkToggleButton*) Button.ShowStars)));
    if (Flags.ShowStars != OldFlags.ShowStars) {
        OldFlags.ShowStars = Flags.ShowStars;
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
        printf("\nplasmastorm: updateStarsUserSettings() User Setting change !!.\n\n");
    }
    //printf("plasmastorm: updateStarsUserSettings() Starts for ShowStars: %i %i.\n",
    //    Flags.ShowStars, gtk_toggle_button_get_active(
    //        GTK_TOGGLE_BUTTON((GtkToggleButton*) Button.ShowStars)));

    //UIDO(NStars, 
    if (Flags.MaxStarCount != OldFlags.MaxStarCount) {
        OldFlags.MaxStarCount = Flags.MaxStarCount;
        initStarsModuleArrays();
        clearStormWindow();
        Flags.mHaveFlagsChanged++;
    }

    if (hasAppScaleChangedFrom(&mPreviousAppScale)) {
        initStarsModuleSurfaces();
        initStarsModuleArrays();
    }
}
