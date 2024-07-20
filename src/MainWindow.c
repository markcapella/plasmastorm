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
#include <glib.h>
#include <glib/gprintf.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// X11 headers.
#include <X11/Intrinsic.h>

// GTK headers.
#include <gtk/gtk.h>

// Plasmastorm headers.
#include "plasmastorm.h"

#include "Application.h"
#include "ClockHelper.h"
#include "generatedGladeIncludes.h"
#include "MainWindow.h"
#include "mygettext.h"
#include "pixmaps.h"
#include "Prefs.h"
#include "safeMalloc.h"
#include "Storm.h"
#include "utils.h"
#include "versionHelper.h"
#include "Windows.h"


/***********************************************************
 * Module Method stubs.
 */

/***********************************************************
 * Module consts.
 */
#ifdef __cplusplus
#define MODULE_EXPORT extern "C" G_MODULE_EXPORT
#else
#define MODULE_EXPORT G_MODULE_EXPORT
#endif

#define PLASMASTORM_TITLE_STRING ""

static GtkWidget* mMainWindow = NULL;
static GtkBuilder* builder = NULL;

static char* lang[100];

static bool mIsUserThreadRunning = false;
static bool mOnUserThread = true;

static GtkCssProvider* mCSSProvider = NULL;
static GtkStyleContext* mStyleContext = NULL;


/** *********************************************************************
 ** Create all button stubs.
 **/
static struct _button {

    GtkWidget* ShowStormItems;
    GtkWidget* StormItemColor1;
    GtkWidget* StormItemColor2;
    GtkWidget* ShapeSizeFactor;

    GtkWidget* StormItemSpeedFactor;
    GtkWidget* StormItemCountMax;
    GtkWidget* StormSaturationFactor;

    GtkWidget* ShowStars;
    GtkWidget* MaxStarCount;

    GtkWidget* ShowWind;
    GtkWidget* WhirlFactor;
    GtkWidget* WhirlTimer;

    GtkWidget* KeepFallenOnWindows;
    GtkWidget* MaxWindowFallenDepth;
    GtkWidget* WindowFallenTopOffset;

    GtkWidget* KeepFallenOnDesktop;
    GtkWidget* MaxDesktopFallenDepth;
    GtkWidget* DesktopFallenTopOffset;

    GtkWidget* ShowBlowoff;
    GtkWidget* BlowOffFactor;

    GtkWidget* CpuLoad;
    GtkWidget* Transparency;
    GtkWidget* Scale;

    GtkWidget* AllWorkspaces;

} Button;


/** *********************************************************************
 ** MainWindow class getter / setters.
 **/
GtkWidget* getMainWindow() {
    return mMainWindow;
}

/** *********************************************************************
 ** Main UI Form control.
 **/
void createMainWindown() {
    mIsUserThreadRunning = true;

    builder = gtk_builder_new_from_string(mStringBuilder, -1);
    #ifdef HAVE_GETTEXT
        gtk_builder_set_translation_domain(builder,
            PLASMASTORM_TEXT_DOMAIN);
    #endif
    gtk_builder_connect_signals(builder, NULL);

    // Main application window.
    mMainWindow = GTK_WIDGET(gtk_builder_get_object(
        builder, "id-MainWindow"));

    g_signal_connect(G_OBJECT(mMainWindow), "window-state-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "configure-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "focus-in-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "focus-out-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "map-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "unmap-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "property-notify-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);
    g_signal_connect(G_OBJECT(mMainWindow), "visibility-notify-event",
        G_CALLBACK(handleMainWindowStateEvents), NULL);

    // Set theme & title & icon, then show window.
    applyUICSSTheme();

    gtk_window_set_icon_from_file(GTK_WINDOW(mMainWindow),
        "/usr/share/icons/hicolor/48x48/apps/"
        "plasmadashboardicon.png", NULL);

    gtk_window_set_title(GTK_WINDOW(mMainWindow),
        PLASMASTORM_TITLE_STRING);

    gtk_widget_show_all(mMainWindow);

    // More initialization.
    init_buttons();
    connectAllButtonSignals();
    init_pixmaps();
    set_buttons();

    /**
     ** StormShapes ComboBox.
     **/
    GtkComboBoxText* shapesComboBox =
        GTK_COMBO_BOX_TEXT(gtk_builder_get_object(
        builder, "id-ComboStormShape"));

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Random Snow");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Tiny Snow");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Leaves");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Rain");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Bubbles");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Hearts");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Diamonds");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(
        shapesComboBox), "Music Notes");

    // Set Active selection from pref.
    gtk_combo_box_set_active(GTK_COMBO_BOX(
        shapesComboBox), Flags.ComboStormShape);

    // Random Snow has sizing available.
    removeSliderNotAvailStyleClass();
    if (Flags.ComboStormShape != 0) {
        addSliderNotAvailStyleClass();
    }

    g_signal_connect(shapesComboBox, "changed",
        G_CALLBACK(onSelectedStormShapeBox), NULL);

    /**
     ** LangButton Combo box.
     **/
    GtkComboBoxText* LangButton =
        GTK_COMBO_BOX_TEXT(gtk_builder_get_object(
        builder, "id-Lang"));

    #define NS_BUFFER 1024
    char sbuffer[NS_BUFFER];

    strcpy(sbuffer, _("Available languages are:\n    "));
    strcat(sbuffer, LANGUAGES);
    strcat(sbuffer, ".\n");
    strcat(sbuffer, _("Use \"sys\" for your default language.\n"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(LangButton), sbuffer);

    gtk_combo_box_text_remove_all(LangButton);
    gtk_combo_box_text_append_text(LangButton, "sys");
    lang[0] = strdup("sys");
    int nlang = 1;

    char* languages = strdup(LANGUAGES);
    char* token = strtok(languages, " ");

    while (token != NULL) {
        lang[nlang] = strdup(token);
        gtk_combo_box_text_append_text(LangButton, lang[nlang]);
        nlang++;
        token = strtok(NULL, " ");
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(LangButton), 0);

    for (int i = 0; i < nlang; i++) {
        if (!strcmp(lang[i], Flags.Language)) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(LangButton), i);
            break;
        }
    }

    g_signal_connect(LangButton, "changed",
        G_CALLBACK(onSelectedLanguageBox), NULL);
}

/** *********************************************************************
 ** Get all button form ID & accessors.
 **/
static void getAllButtonFormIDs() {

    Button.ShowStormItems = (GtkWidget*)
        gtk_builder_get_object(builder, "id-ShowStormItems");
    Button.StormItemColor1 = (GtkWidget*)
        gtk_builder_get_object(builder, "id-StormItemColor1");
    Button.StormItemColor2 = (GtkWidget*)
        gtk_builder_get_object(builder, "id-StormItemColor2");

    Button.ShapeSizeFactor = (GtkWidget*)
        gtk_builder_get_object(builder, "id-ShapeSizeFactor");
    Button.StormItemSpeedFactor = (GtkWidget*)
        gtk_builder_get_object(builder, "id-StormItemSpeedFactor");
    Button.StormItemCountMax = (GtkWidget*)
        gtk_builder_get_object(builder, "id-StormItemCountMax");
    Button.StormSaturationFactor = (GtkWidget*)
        gtk_builder_get_object(builder, "id-StormSaturationFactor");

    Button.ShowStars = (GtkWidget*)
        gtk_builder_get_object(builder, "id-ShowStars");
    Button.MaxStarCount = (GtkWidget*)
        gtk_builder_get_object(builder, "id-MaxStarCount");

    Button.ShowWind = (GtkWidget*)
        gtk_builder_get_object(builder, "id-ShowWind");
    Button.WhirlFactor = (GtkWidget*)
        gtk_builder_get_object(builder, "id-WhirlFactor");
    Button.WhirlTimer = (GtkWidget*)
        gtk_builder_get_object(builder, "id-WhirlTimer");

    Button.KeepFallenOnWindows = (GtkWidget*)
        gtk_builder_get_object(builder, "id-KeepFallenOnWindows");
    Button.MaxWindowFallenDepth = (GtkWidget*)
        gtk_builder_get_object(builder, "id-MaxWindowFallenDepth");
    Button.WindowFallenTopOffset = (GtkWidget*)
        gtk_builder_get_object(builder, "id-WindowFallenTopOffset");

    Button.KeepFallenOnDesktop = (GtkWidget*)
        gtk_builder_get_object(builder, "id-KeepFallenOnDesktop");
    Button.MaxDesktopFallenDepth = (GtkWidget*)
        gtk_builder_get_object(builder, "id-MaxDesktopFallenDepth");
    Button.DesktopFallenTopOffset = (GtkWidget*)
        gtk_builder_get_object(builder, "id-DesktopFallenTopOffset");

    Button.ShowBlowoff = (GtkWidget*)
        gtk_builder_get_object(builder, "id-ShowBlowoff");
    Button.BlowOffFactor = (GtkWidget*)
        gtk_builder_get_object(builder, "id-BlowOffFactor");

    Button.CpuLoad = (GtkWidget*)
        gtk_builder_get_object(builder, "id-CpuLoad");
    Button.Transparency = (GtkWidget*)
        gtk_builder_get_object(builder, "id-Transparency");
    Button.Scale = (GtkWidget*)
        gtk_builder_get_object(builder, "id-Scale");

    Button.AllWorkspaces = (GtkWidget*)
        gtk_builder_get_object(builder, "id-AllWorkspaces");
}

/** *********************************************************************
 ** Define all button SET methods.
 **/
void
onChangedShowStormItems(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.ShowStormItems = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedStormItemColor1() {
    if (!mOnUserThread) {
        return;
    }
    startQPickerDialog("StormItemColor1TAG",
        Flags.StormItemColor1);
}

void
onChangedStormItemColor2() {
    if (!mOnUserThread) {
        return;
    }
    startQPickerDialog("StormItemColor2TAG",
        Flags.StormItemColor2);
}

void
onChangedShapeSizeFactor(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.ShapeSizeFactor = lrint(value);
}

void
onChangedStormItemSpeedFactor(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.StormItemSpeedFactor = lrint(value);
}

void
onChangedStormItemCountMax(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.StormItemCountMax = lrint(value);
}

void
onChangedStormItemSaturationFactor(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.StormSaturationFactor = lrint(value);
}

void
onClickedShowStars(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }

    Flags.ShowStars = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedMaxStarCount(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.MaxStarCount = lrint(value);
}

void
onChangedShowWind(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.ShowWind = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedWhirlFactor(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.WhirlFactor = lrint(value);
}

void
onChangedWhirlTimer(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.WhirlTimer = lrint(value);
}

void
onChangedKeepFallenOnWindows(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.KeepFallenOnWindows = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedMaxWindowFallenDepth(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.MaxWindowFallenDepth = lrint(value);
}

void
onChangedWindowFallenTopOffset(GtkWidget* w) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(((((GtkRange*)
        (void*) g_type_check_instance_cast((GTypeInstance*) ((w)),
        ((gtk_range_get_type ())))))));

    /* Negative Here */
    Flags.WindowFallenTopOffset = -1 * lrint(value);
}

void
onChangedKeepFallenOnBottom(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.KeepFallenOnDesktop = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedMaxDesktopFallenDepth(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.MaxDesktopFallenDepth = lrint(value);
}

void
onChangedDesktopFallenTopOffset(GtkWidget* w) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(((((GtkRange*)
        (void*) g_type_check_instance_cast((GTypeInstance*) ((w)),
        ((gtk_range_get_type ())))))));

    /* Negative Here */
    Flags.DesktopFallenTopOffset = -1 * lrint(value);
}

void
onChangedShowBlowoff(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.ShowBlowoff = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

void
onChangedBlowOffFactor(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.BlowOffFactor = lrint(value);
}

void
onChangedCpuLoad(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.CpuLoad = lrint(value);
}

void
onChangedTransparency(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.Transparency = lrint(value);
}

void
onChangedScale(GtkWidget* slider) {
    if (!mOnUserThread) {
        return;
    }
    const gdouble value = gtk_range_get_value(
        GTK_RANGE(slider));
    Flags.Scale = lrint(value);
}

void
onChangedAllWorkspaces(GtkWidget* toggleButton) {
    if (!mOnUserThread) {
        return;
    }
    Flags.AllWorkspaces = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(toggleButton));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/** *********************************************************************
 ** Init all buttons values.
 **/
void setWidgetValuesFromPrefs() {
    GdkRGBA color;

    gtk_toggle_button_set_active((GtkToggleButton*)
        Button.ShowStormItems, Flags.ShowStormItems);
    gdk_rgba_parse(&color, Flags.StormItemColor1);
    gtk_widget_override_background_color(Button.StormItemColor1,
        GTK_STATE_FLAG_NORMAL, &color);
    gdk_rgba_parse(&color, Flags.StormItemColor2);
    gtk_widget_override_background_color(Button.StormItemColor2,
        GTK_STATE_FLAG_NORMAL, &color);

    gtk_range_set_value((GtkRange*) Button.ShapeSizeFactor,
        Flags.ShapeSizeFactor);
    gtk_range_set_value((GtkRange*) Button.StormItemSpeedFactor,
        Flags.StormItemSpeedFactor);
    gtk_range_set_value((GtkRange*) Button.StormItemCountMax,
        Flags.StormItemCountMax);
    gtk_range_set_value((GtkRange*) Button.StormSaturationFactor,
        Flags.StormSaturationFactor);

    gtk_toggle_button_set_active((GtkToggleButton*) Button.ShowStars,
        Flags.ShowStars);
    gtk_range_set_value((GtkRange*) Button.MaxStarCount,
        Flags.MaxStarCount);

    gtk_toggle_button_set_active((GtkToggleButton*) Button.ShowWind,
        Flags.ShowWind);
    gtk_range_set_value((GtkRange*) Button.WhirlFactor,
        Flags.WhirlFactor);
    gtk_range_set_value((GtkRange*) Button.WhirlTimer,
        Flags.WhirlTimer);

    gtk_toggle_button_set_active((GtkToggleButton*)
        Button.KeepFallenOnWindows, Flags.KeepFallenOnWindows);
    gtk_range_set_value((GtkRange*) Button.MaxWindowFallenDepth,
        Flags.MaxWindowFallenDepth);
    /* Negative Here */
    gtk_range_set_value((GtkRange*) Button.WindowFallenTopOffset,
        -Flags.WindowFallenTopOffset);

    gtk_toggle_button_set_active((GtkToggleButton*)
        Button.KeepFallenOnDesktop, Flags.KeepFallenOnDesktop);
    gtk_range_set_value((GtkRange*) Button.MaxDesktopFallenDepth,
        Flags.MaxDesktopFallenDepth);
    /* Negative Here */
    gtk_range_set_value((GtkRange*) Button.DesktopFallenTopOffset,
        -Flags.DesktopFallenTopOffset);

    gtk_toggle_button_set_active((GtkToggleButton*) Button.ShowBlowoff,
        Flags.ShowBlowoff);
    gtk_range_set_value((GtkRange*) Button.BlowOffFactor,
        Flags.BlowOffFactor);

    gtk_range_set_value((GtkRange*) Button.CpuLoad,
        Flags.CpuLoad);
    gtk_range_set_value((GtkRange*) Button.Transparency,
        Flags.Transparency);
    gtk_range_set_value((GtkRange*) Button.Scale,
        Flags.Scale);

    gtk_toggle_button_set_active((GtkToggleButton*) Button.AllWorkspaces,
        Flags.AllWorkspaces);
}
#pragma GCC diagnostic pop

/** *********************************************************************
 ** Hook all buttons to their action methods.
 **/
void connectAllButtonSignals() {

    g_signal_connect(G_OBJECT(Button.ShowStormItems), "toggled",
        G_CALLBACK(onChangedShowStormItems), NULL);
    g_signal_connect(G_OBJECT(Button.StormItemColor1), "toggled",
        G_CALLBACK(onChangedStormItemColor1), NULL);
    g_signal_connect(G_OBJECT(Button.StormItemColor2), "toggled",
        G_CALLBACK(onChangedStormItemColor2), NULL);

    g_signal_connect(G_OBJECT(Button.ShapeSizeFactor), "value-changed",
        G_CALLBACK(onChangedShapeSizeFactor), NULL);
    g_signal_connect(G_OBJECT(Button.StormItemSpeedFactor), "value-changed",
        G_CALLBACK(onChangedStormItemSpeedFactor), NULL);
    g_signal_connect(G_OBJECT(Button.StormItemCountMax), "value-changed",
        G_CALLBACK(onChangedStormItemCountMax), NULL);
    g_signal_connect(G_OBJECT(Button.StormSaturationFactor), "value-changed",
        G_CALLBACK(onChangedStormItemSaturationFactor), NULL);

    g_signal_connect(G_OBJECT(Button.ShowStars), "toggled",
        G_CALLBACK(onClickedShowStars), NULL);
    g_signal_connect(G_OBJECT(Button.MaxStarCount), "value-changed",
        G_CALLBACK(onChangedMaxStarCount), NULL);

    g_signal_connect(G_OBJECT(Button.ShowWind), "toggled",
        G_CALLBACK(onChangedShowWind), NULL);
    g_signal_connect(G_OBJECT(Button.WhirlFactor), "value-changed",
        G_CALLBACK(onChangedWhirlFactor), NULL);
    g_signal_connect(G_OBJECT(Button.WhirlTimer), "value-changed",
        G_CALLBACK(onChangedWhirlTimer), NULL);

    g_signal_connect(G_OBJECT(Button.KeepFallenOnWindows), "toggled",
        G_CALLBACK(onChangedKeepFallenOnWindows), NULL);
    g_signal_connect(G_OBJECT(Button.MaxWindowFallenDepth), "value-changed",
        G_CALLBACK(onChangedMaxWindowFallenDepth), NULL);
    g_signal_connect(G_OBJECT(Button.WindowFallenTopOffset), "value-changed",
        G_CALLBACK(onChangedWindowFallenTopOffset), NULL);

    g_signal_connect(G_OBJECT(Button.KeepFallenOnDesktop), "toggled",
        G_CALLBACK(onChangedKeepFallenOnBottom), NULL);
    g_signal_connect(G_OBJECT(Button.MaxDesktopFallenDepth), "value-changed",
        G_CALLBACK(onChangedMaxDesktopFallenDepth), NULL);
    g_signal_connect(G_OBJECT(Button.DesktopFallenTopOffset), "value-changed",
        G_CALLBACK(onChangedDesktopFallenTopOffset), NULL);

    g_signal_connect(G_OBJECT(Button.ShowBlowoff), "toggled",
        G_CALLBACK(onChangedShowBlowoff), NULL);
    g_signal_connect(G_OBJECT(Button.BlowOffFactor), "value-changed",
        G_CALLBACK(onChangedBlowOffFactor), NULL);

    g_signal_connect(G_OBJECT(Button.CpuLoad), "value-changed",
        G_CALLBACK(onChangedCpuLoad), NULL);
    g_signal_connect(G_OBJECT(Button.Transparency), "value-changed",
        G_CALLBACK(onChangedTransparency), NULL);
    g_signal_connect(G_OBJECT(Button.Scale), "value-changed",
        G_CALLBACK(onChangedScale), NULL);

    g_signal_connect(G_OBJECT(Button.AllWorkspaces), "toggled",
        G_CALLBACK(onChangedAllWorkspaces), NULL);
}

/** *********************************************************************
 ** Pixmap helpers.
 **/
static void init_hello_pixmaps() {
    GtkImage *aboutPlasmaLogo = GTK_IMAGE(
        gtk_builder_get_object(builder, "id-plasmastormLogo"));
    GdkPixbuf *aboutPlasmaPixbug = gdk_pixbuf_new_from_xpm_data(
        (XPM_TYPE **) mPlasmastormLogoPixmap);

    gtk_image_set_from_pixbuf(aboutPlasmaLogo, aboutPlasmaPixbug);

    g_object_unref(aboutPlasmaPixbug);
}

/** *********************************************************************
 ** Pixmap helpers.
 **/
void init_pixmaps() {
    init_hello_pixmaps();
}

/** *********************************************************************
 ** ComboBox Helpers.
 **/
MODULE_EXPORT
void onSelectedStormShapeBox(GtkComboBoxText *combo,
     __attribute__((unused)) gpointer data) {

    Flags.ComboStormShape = gtk_combo_box_get_active(
        GTK_COMBO_BOX(combo));
    if (Flags.ComboStormShape != OldFlags.ComboStormShape) {
        OldFlags.ComboStormShape = Flags.ComboStormShape;
        Flags.mHaveFlagsChanged++;
    }

    // Random Snow has sizing available.
    removeSliderNotAvailStyleClass();
    if (Flags.ComboStormShape != 0) {
        addSliderNotAvailStyleClass();
    }
}

MODULE_EXPORT
void onSelectedLanguageBox(GtkComboBoxText *combo,
    __attribute__((unused)) gpointer data) {

    int num = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    Flags.Language = strdup(lang[num]);
}

/** *********************************************************************
 ** This sets the var used to display the app version number on
 ** the About UI page.
 **/
static void setVersionNumberForAboutPage() {
    setLabelText(GTK_LABEL(gtk_builder_get_object(builder,
        "id-version")), "plasmastorm-" VERSION);
}

/** *********************************************************************
 ** Page Default Tab button Helpers.
 **/
void setDefaultStormSettings() {
    Flags.ShowStormItems = DefaultFlags.ShowStormItems;

    Flags.ComboStormShape = DefaultFlags.ComboStormShape;
    gtk_combo_box_set_active(GTK_COMBO_BOX(GTK_COMBO_BOX_TEXT(
        gtk_builder_get_object(builder, "id-ComboStormShape"))),
        Flags.ComboStormShape);

    Flags.StormItemColor1 = DefaultFlags.StormItemColor1;
    Flags.StormItemColor2 = DefaultFlags.StormItemColor2;

    Flags.ShapeSizeFactor = DefaultFlags.ShapeSizeFactor;
    Flags.StormItemSpeedFactor = DefaultFlags.StormItemSpeedFactor;
    Flags.StormItemCountMax = DefaultFlags.StormItemCountMax;
    Flags.StormSaturationFactor = DefaultFlags.StormSaturationFactor;

    Flags.ShowStars = DefaultFlags.ShowStars;
    Flags.MaxStarCount = DefaultFlags.MaxStarCount;
}

void setDefaultWindSettings() {
    Flags.ShowWind = DefaultFlags.ShowWind;

    Flags.WhirlFactor = DefaultFlags.WhirlFactor;
    Flags.WhirlTimer = DefaultFlags.WhirlTimer;
}

void setDefaultFallenSettings() {
    Flags.KeepFallenOnWindows = DefaultFlags.KeepFallenOnWindows;
    Flags.MaxWindowFallenDepth = DefaultFlags.MaxWindowFallenDepth;
    Flags.WindowFallenTopOffset = DefaultFlags.WindowFallenTopOffset;

    Flags.KeepFallenOnDesktop = DefaultFlags.KeepFallenOnDesktop;
    Flags.MaxDesktopFallenDepth = DefaultFlags.MaxDesktopFallenDepth;
    Flags.DesktopFallenTopOffset = DefaultFlags.DesktopFallenTopOffset;

    Flags.ShowBlowoff = DefaultFlags.ShowBlowoff;
    Flags.BlowOffFactor = DefaultFlags.BlowOffFactor;
}

void setDefaultAdvancedSettings() {
    Flags.CpuLoad = DefaultFlags.CpuLoad;
    Flags.Transparency = DefaultFlags.Transparency;
    Flags.Scale = DefaultFlags.Scale;

    Flags.AllWorkspaces = DefaultFlags.AllWorkspaces;
}

/** *********************************************************************
 ** Helpers.
 **/
void set_buttons() {
    mOnUserThread = false;
    setWidgetValuesFromPrefs();
    mOnUserThread = true;
}

/** *********************************************************************
 ** Helpers.
 **/
void init_buttons() {
    getAllButtonFormIDs();

    setVersionNumberForAboutPage();
}

/** *********************************************************************
 ** Set the UI Main Window Sticky Flag.
 **/
void setMainWindowSticky(bool stickyFlag) {
    if (!mIsUserThreadRunning) {
        return;
    }
    if (stickyFlag) {
        gtk_window_stick(GTK_WINDOW(mMainWindow));
    } else {
        gtk_window_unstick(GTK_WINDOW(mMainWindow));
    }
}

/** *********************************************************************
 ** CSS related code for MainWindow styling.
 **/
void applyUICSSTheme() {
    if (mCSSProvider) {
        return;
    }

    const char* MAIN_UI_CSS =
        // App Title / Header bar.
        "headerbar                 { background:       #fff7a2; }"

        // App background - Lightest Orange.
        "stack                     { background:       #fdf7eb; }"

        // Light Orange.
        "button.toggle             { background: #fbefd5; }"
        "button.radio              { background: #fbefd5; }"

        // Light Orange 2.
        "button:active             { background: #fbf0da; }"
        "scale trough              { background: #fbf0da; }"

        // Bright Orange.
        "button:checked            { background: #fcdb94; }"

        // Slider trough colors.
        "scale trough              { background: #c3c3c3; }"
        "scale trough highlight    { background: #8a8a8a; }"

        // Size Sslider not avail is gray.
        ".isNotAvail  .sizeSlider     { background: #e3e3e3; }"

        // CPU Busy Warnings - Hot Pink Danger.
        ".mAppBusy stack           { background: #ff74ff; }"
        ".mAppBusy .cpuload slider { background: #ff74ff; }"

        // Misc.
        "scale                     { padding:    1em;     }"
        "button.radio              { min-width:  10px;    }"
        ".button                   { background: #CCF0D8; }"
    ;

    // Get new CSS provider, load with our CSS string & apply.
    mCSSProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(mCSSProvider,
        MAIN_UI_CSS, -1, NULL);
    applyCSSToWindow(mMainWindow, mCSSProvider);

    // Get application style context & set initial.
    mStyleContext = gtk_widget_get_style_context(mMainWindow);
}

/** *********************************************************************
 ** Helper to set the style provider.
 **/
void applyCSSToWindow(GtkWidget* widget,
    GtkCssProvider* cssstyleProvider) {

    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(cssstyleProvider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // For container widgets, apply to every child.
    if (GTK_IS_CONTAINER(widget)) {
        gtk_container_forall(GTK_CONTAINER(widget),
            (GtkCallback) applyCSSToWindow, cssstyleProvider);
    }
}

/** *********************************************************************
 ** "Busy" Style class getter / setters.
 **/
void addBusyStyleClass() {
    if (!mIsUserThreadRunning) {
        return;
    }
    gtk_style_context_add_class(mStyleContext, "mAppBusy");
}

void removeBusyStyleClass() {
    if (!mIsUserThreadRunning) {
        return;
    }
    gtk_style_context_remove_class(mStyleContext, "mAppBusy");
}

/** *********************************************************************
 ** "Busy" Style class getter / setters.
 **/
void addSliderNotAvailStyleClass() {
    if (!mIsUserThreadRunning) {
        return;
    }
    gtk_style_context_add_class(mStyleContext, "isNotAvail");
}

void removeSliderNotAvailStyleClass() {
    if (!mIsUserThreadRunning) {
        return;
    }
    gtk_style_context_remove_class(mStyleContext, "isNotAvail");
}

/** *********************************************************************
 ** ... .
 **/
char* ui_gtk_version() {
    static char versionString[20];

    snprintf(versionString, 20, "%d.%d.%d",
        gtk_get_major_version(),
        gtk_get_minor_version(),
        gtk_get_micro_version());

    return versionString;
}

/** *********************************************************************
 ** returns:
 ** 0: gtk version in use too low
 ** 1: gtk version in use OK
 **/
char* ui_gtk_required() {
    static char s[20];
    snprintf(s, 20, "%d.%d.%d", GTK_MAJOR, GTK_MINOR, GTK_MICRO);
    return s;
}

/** *********************************************************************
 ** ... .
 **/
int isGtkVersionValid() {
    if ((int) gtk_get_major_version() > GTK_MAJOR) {
        return 1;
    }
    if ((int) gtk_get_major_version() < GTK_MAJOR) {
        return 0;
    }
    if ((int) gtk_get_minor_version() > GTK_MINOR) {
        return 1;
    }
    if ((int) gtk_get_minor_version() < GTK_MINOR) {
        return 0;
    }
    if ((int) gtk_get_micro_version() >= GTK_MICRO) {
        return 1;
    }

    return 0;
}

/** *********************************************************************
 ** ... .
 **/
void setLabelText(GtkLabel *label, const gchar *str) {
    if (mIsUserThreadRunning) {
        gtk_label_set_text(label, str);
    }
}

MODULE_EXPORT
void onClickedQuitApplication() {
    Flags.shutdownRequested = true;
}

/** *********************************************************************
 ** ui.glade Form Helpers - All DEFAULT button actions.
 **/

MODULE_EXPORT
void onClickedSetAllDefaults() {
    setDefaultStormSettings();
    setDefaultWindSettings();
    setDefaultFallenSettings();
    setDefaultAdvancedSettings();
}

MODULE_EXPORT
void onClickedSetStormDefaults() {
    setDefaultStormSettings();
}
MODULE_EXPORT
void onClickedSetWindDefaults() {
    setDefaultWindSettings();
}
MODULE_EXPORT
void onClickedSetFallenDefaults() {
    setDefaultFallenSettings();
}
MODULE_EXPORT
void onClickedSetAdvancedDefaults() {
    setDefaultAdvancedSettings();
}

/** *********************************************************************
 ** ...
 **/
void respondToLanguageSettingsChanges() {
    if (!strcmp(Flags.Language, OldFlags.Language)) {
        return;
    }
    free(OldFlags.Language);
    OldFlags.Language = strdup(Flags.Language);
    Flags.mHaveFlagsChanged++;

    printf("\nplasmastorm: Restarting due to language change.\n");

    setLanguageEnvironmentVar();
    mGlobal.languageChangeRestart = true;
    Flags.shutdownRequested = true;
}

/** *********************************************************************
 ** ...
 **/
void setLanguageEnvironmentVar() {
    if (!strcmp(Flags.Language, "sys")) {
        unsetenv("LANGUAGE");
    } else {
        setenv("LANGUAGE", Flags.Language, 1);
    }
}

/** *********************************************************************
 ** Main WindowState event handler.
 **/
gboolean handleMainWindowStateEvents(
    __attribute__((unused)) GtkWidget* widget,
    __attribute__((unused)) GdkEventWindowState* event,
    __attribute__((unused)) gpointer user_data) {

    return false;
}
