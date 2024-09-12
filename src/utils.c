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
#include <gsl/gsl_sort.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/Intrinsic.h>

#include <gtk/gtk.h>

#include "Prefs.h"
#include "mygettext.h"
#include "plasmastorm.h"
#include "safeMalloc.h"
#include "utils.h"
#include "versionHelper.h"
#include "Windows.h"
#include "xdo.h"

#define BACKTRACE_BUFFER_SIZE 100


/** *********************************************************************
 ** This method ...
 **/
void traceback() {
    #ifdef TRACEBACK_AVAILALBLE
        // see man backtrace
        void *buffer[BACKTRACE_BUFFER_SIZE];
        int nptrs = backtrace(buffer, BACKTRACE_BUFFER_SIZE);
        backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);
    #endif
}

/** *********************************************************************
 ** This method ...
 **/
int IsReadableFile(char* path) {
    if (!path || access(path, R_OK) != 0) {
        return 0;
    }
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

/** *********************************************************************
 ** This method ...
 **/
FILE* HomeOpen(const char *file, const char *mode, char **path) {
    char *h = getenv("HOME");
    if (h == NULL) {
        return NULL;
    }
    char *home = strdup(h);
    (*path) = (char *)malloc(strlen(home) + strlen(file) + 2);
    strcpy(*path, home);
    strcat(*path, "/");
    strcat(*path, file);
    FILE *f = fopen(*path, mode);
    free(home);
    return f;
}

/** *********************************************************************
 ** This method removes all our storm drawings.
 **/
void clearStormWindow() {
    XClearArea(mGlobal.display, mGlobal.StormWindow,
        0, 0, 0, 0, True);
    XFlush(mGlobal.display);
}

/** *********************************************************************
 ** This method ...
 **/
ssize_t mywrite(int fd, const void *buf, size_t count) {
    const size_t m = 4096; // max per write
    size_t w = 0;          // # written chars
    char *b = (char *)buf;

    while (w < count) {
        size_t l = count - w;
        if (l > m) {
            l = m;
        }
        ssize_t x = write(fd, b + w, l);
        if (x < 0) {
            return -1;
        }
        w += x;
    }

    return 0;
}

/** *********************************************************************
 ** Module MAINLOOP methods.
 **/
void sanelyCheckAndClearDisplayArea(Display *dsp, Window win,
    int x, int y, int w, int h, Bool exposures) {

    if (w == 0 || h == 0 ||
        w < 0 || h < 0 ||
        w > 20000 || h > 20000) {
        traceback();
        return;
    }

    XClearArea(dsp, win, x, y, w, h, exposures);
}

/** *********************************************************************
 ** This method ...
 **/
float sq2(float x, float y) {
    return x * x + y * y;
}

float sq3(float x, float y, float z) {
    return x * x + y * y + z * z;
}

float getWindDirection(float x) {
    if (x > 0) {
        return 1.0f;
    }
    if (x < 0) {
        return -1.0f;
    }

    return 0.0f;
}

/** *********************************************************************
 ** This method ...
 **/
int ValidColor(const char *colorName) {
    XColor scrncolor;
    XColor exactcolor;

    return (XAllocNamedColor(mGlobal.display, DefaultColormap(
        mGlobal.display, DefaultScreen(mGlobal.display)),
        colorName, &scrncolor, &exactcolor));
}

/** *********************************************************************
 ** This method ...
 **/
Pixel AllocNamedColor(const char *colorName, Pixel dfltPix) {
    XColor scrncolor;
    XColor exactcolor;

    if (XAllocNamedColor(mGlobal.display, DefaultColormap(
        mGlobal.display, DefaultScreen(mGlobal.display)),
        colorName, &scrncolor, &exactcolor)) {
        return scrncolor.pixel;
    }

    return dfltPix;
}

/** *********************************************************************
 ** This method ...
 **/
Pixel IAllocNamedColor(const char *colorName, Pixel dfltPix) {
    return AllocNamedColor(colorName, dfltPix) | 0xff000000;
}

/** *********************************************************************
 ** This method ...
 **/
int randint(int m) {
    return (m <= 0) ? 0 : drand48() * m;
}

/** *********************************************************************
 ** This method ...
 **/
guint addMethodToMainloop(gint prio, float time,
    GSourceFunc func) {

    return g_timeout_add_full(prio, (int) 1000 * (time *
        (0.95 + 0.1 * drand48())), func, NULL, NULL);
}

/** *********************************************************************
 ** This method ...
 **/
guint addMethodWithArgToMainloop(gint prio, float time,
    GSourceFunc func, gpointer datap) {

    return g_timeout_add_full(prio, (int) 1000 * (time) *
        (0.95 + 0.1 * drand48()), func, datap, NULL);
}

/** *********************************************************************
 ** This method ...
 **/
void remove_from_mainloop(guint *tag) {
    if (*tag) {
        g_source_remove(*tag);
    }
    *tag = 0;
}

/** *********************************************************************
 ** This method ...
 **/
void paintCairoContextWithAlpha(cairo_t* cc, double alpha) {
    if (alpha > 0.9) {
        cairo_paint(cc);
    } else {
        cairo_paint_with_alpha(cc, alpha);
    }
}

/** *********************************************************************
 ** This method ...
 **/
int is_little_endian(void) {
    volatile int endiantest = 1;
    return (*(char *)&endiantest) == 1;
}

/** *********************************************************************
 ** This method ...
 **/
void rgba2color(GdkRGBA *c, char **s) {
    *s = (char *)malloc(8);
    sprintf(*s, "#%02lx%02lx%02lx", lrint(c->red * 255), lrint(c->green * 255),
        lrint(c->blue * 255));
}

/** *********************************************************************
 ** This method ...
 **/
int hasAppScaleChangedFrom(int* prevscale) {
    const int newscale = (const int)
        (Flags.Scale * mGlobal.WindowScale);

    if (*prevscale != (newscale)) {
        *prevscale = newscale;
        return true;
    }
    return false;
}

/** *********************************************************************
 ** Helper methods to create sorted n random numbers.
 **
 ** double *a : The array to be filled.
 ** int n     :  The number of items in array.
 ** double d  : minimum distance between items.
 ** unsigned short* seed: NULL: use drand48()
 **/
void randomuniqarray(double *a, int n, double d,
    unsigned short *seed) {

    if (seed) {
        for (int i = 0; i < n; i++) {
            a[i] = erand48(seed);
        }
    } else {
        for (int i = 0; i < n; i++) {
            a[i] = drand48();
        }
    }

    gsl_sort(a, 1, n);

    int changes = 0;
    while (1) {
        int changed = 0;

        for (int i = 0; i < n - 1; i++) {
            // draw a new random number for a[i]
            if (fabs(a[i + 1] - a[i]) < d) {
                changed = 1;
                if (seed) {
                    a[i] = erand48(seed);
                } else {
                    a[i] = drand48();
                }
            }
        }
        if (!changed) {
            return;
        }

        // failure to create proper array, fill it with
        // equidistant numbers in stead
        if (++changes > 100) {
            double s = 1.0 / n;
            for (int i = 0; i < n; i++) {
                a[i] = i * s;
            }

            return;
        }

        gsl_sort(a, 1, n);
    }
}

/** *********************************************************************
 ** Helper method inspired by Mr. Gauss, and adapted for app
 **/
float gaussf(float x, float mu, float sigma) {

    float y = (x - mu) / sigma;
    float y2 = y * y;
    return expf(-y2);
}

/** *********************************************************************
 ** Helper method guess language. Return string like "en",
 ** "nl" or NULL if no language can be found.
 **/
char* getLanguageFromEnvStrings() {
    const char *candidateStrings[] = {"LANGUAGE", "LANG", "LC_ALL",
        "LC_MESSAGES", "LC_NAME", "LC_TIME", NULL};

    char *a;
    char *b;

    int i = 0;
    while (candidateStrings[i]) {
        a = getenv(candidateStrings[i]);
        if (a && strlen(a) > 0) {
            b = strdup(a);
            char *p = strchr(b, '_');
            if (p) {
                *p = 0;
                return b;
            } else {
                return b;
            }

            free(b);
        }
        ++i;
    }

    return NULL;
}

/** *********************************************************************
 ** Helper methods ...
 **/
// find largest window with name
Window largest_window_with_name(xdo_t* myxdo,
    const char* name) {

    // Set search req.
    xdo_search_t search;
    memset(&search, 0, sizeof(xdo_search_t));
    search.searchmask = SEARCH_NAME;
    search.winname = name;
    search.require = SEARCH_ANY;
    search.max_depth = 4;
    search.limit = 0;

    // Set result fields and go.
    Window* windowsArray = NULL;
    unsigned int nwindows;
    doXDOWindowSearch(myxdo, &search,
        &windowsArray, &nwindows);

    Window resultWindow = 0;
    unsigned int maxsize = 0;
    for (unsigned int i = 0; i < nwindows; i++) {
        // Get this windowItem area.
        unsigned int width, height;
        xdo_get_window_size(myxdo, windowsArray[i], &width, &height);
        const unsigned int windowArea = width * height;

        // If it's the biggest,
        // save its size, and Window.
        if (windowArea > maxsize) {
            maxsize = windowArea;
            resultWindow = windowsArray[i];
        }
    }

    if (windowsArray) {
        free(windowsArray);
    }

    return resultWindow;
}

/** *********************************************************************
 ** Helper methods for Color.
 **/
extern GdkRGBA getRGBFromString(char* colorString) {
    GdkRGBA result;
    gdk_rgba_parse(&result, colorString);
    return result;
}

/** *********************************************************************
 ** This method pretty-prints to log the app name, version, Author.
 **/
void logAppVersion() {
    const int numberOfStars = strlen(PACKAGE_STRING) + 4;

    printf("\n   ");
    for (int i = 0; i < numberOfStars; i++) {
        printf("*");
    }
    printf("\n   * %s *\n", PACKAGE_STRING);
    printf("   ");
    for (int i = 0; i < numberOfStars; i++) {
        printf("*");
    }

    printf("\n\n%s\n", VERSIONBY);
}
