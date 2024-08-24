# -copyright-
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
SUBDIRS = Pixmaps

TOCC = $(top_srcdir)/src/tocc.sh

# /usr/local
gamesdir = $(exec_prefix)/games
games_PROGRAMS = plasmastorm

# Qt Dev package
QT_CDEFINES  = -DQT_NO_DEBUG -DQT_CORE_LIB -DQT_GUI_LIB -DQT_WIDGETS_LIB
QT_CFLAGS    = -pipe -O2 -Wall -Wextra -D_REENTRANT -fPIC $(QT_CDEFINES)
QT_CINCLUDES = -I/usr/include/x86_64-linux-gnu/qt5 -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
	-I/usr/include/x86_64-linux-gnu/qt5/QtGui -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets
QT_LIBS      = /usr/lib/x86_64-linux-gnu/libQt5Core.so /usr/lib/x86_64-linux-gnu/libQt5Gui.so \
	/usr/lib/x86_64-linux-gnu/libQt5Widgets.so

# MAKE_DEBUGGABLE_BY_GDB = "-g -O0 -v -da -Q"
plasmastorm_CPPFLAGS = $(QT_CFLAGS) $(QT_CINCLUDES) \
	$(GTK_CFLAGS) $(X11_CFLAGS) $(GSL_CFLAGS) -DLOCALEDIR=\"$(LOCALEDIR)\" \
	-DLANGUAGES='"$(LANGUAGES)"'
plasmastorm_LDADD = libxdo.a $(GTK_LIBS) $(QT_LIBS) $(X11_LIBS) \
	-lXfixes $(GSL_LIBS) $(LIBINTL) 
libxdo_a_CPPFLAGS = $(X11_CFLAGS)

if USE_NLS
   LANGUAGES != for l in `sed 's/\#.*//' $(top_srcdir)/po/LINGUAS`; \
		do echo $$l; done
   plasmastorm_CPPFLAGS += -DENABLE_NLS=1
else
   LANGUAGES = 
endif

noinst_LIBRARIES = libxdo.a
libxdo_a_SOURCES = xdo.h xdo.c \
	XDOSearch.h XDOSearch.c \
	XDOSymbolMap.h

plasmastorm_SOURCES = \
		Application.c Blowoff.c ClockHelper.c ColorPicker.cpp \
		Fallen.c hashTableHelper.cpp loadmeasure.c mainstub.cpp \
		MainWindow.c MsgBox.cpp pixmaps.c Prefs.c safeMalloc.c \
		splineHelper.c Stars.c Storm.c StormWindow.c ui.glade \
		utils.c Wind.c Windows.c x11WindowHelper.c xpmHelper.c

nodist_plasmastorm_SOURCES = generatedGladeIncludes.h generatedIncludes.h

BUILT_SOURCES = generatedGladeIncludes.h generatedIncludes.h $(tarfile_inc)

tarfile = $(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz

$(tarfile):
	echo "Dummy tarfile." > $@

EXTRA_DIST = generateShapeIncludes.sh \
	generateGladeIncludes.sh \
	tocc.sh 
generatedGladeIncludes.h: ui.glade $(top_srcdir)/src/generateGladeIncludes.sh
	@echo ">>> GLADE ui.glade Creating $@ from $<"
	$(top_srcdir)/src/generateGladeIncludes.sh $(top_srcdir)

generatedIncludes.h: Pixmaps
	@echo ">>> Pixmaps Creating $@"
	$(top_srcdir)/src/generateShapeIncludes.sh $(top_srcdir)

install-data-local:
	cp 'Pixmaps/plasmastorm.png' /usr/share/icons/hicolor/48x48/apps/
	cp 'Pixmaps/plasmastormbox.png' /usr/share/icons/hicolor/48x48/apps/

uninstall-local:
	rm -f /usr/share/icons/hicolor/48x48/apps/plasmastorm.png
	rm -f /usr/share/icons/hicolor/48x48/apps/plasmastormbox.png

TESTS = test1.sh

# usr/local/share
appicondir=$(datadir)/pixmaps
appicon_DATA=Pixmaps/plasmastorm.png \
	Pixmaps/plasmastormpicker.png

man6_MANS = plasmastorm.6
plasmastorm.6: plasmastorm
	echo "deprecated" > $@

CLEANFILES = plasmastorm.6 generatedGladeIncludes.h \
	generatedIncludes.h plasmastorm_out_2 \
	plasmastorm_out_3 tarfile.inc
