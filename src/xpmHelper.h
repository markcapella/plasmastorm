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
#pragma once

#include <X11/xpm.h>
#include <gtk/gtk.h>


extern int iXpmCreatePixmapFromData(Display *display,
    Drawable d, const char **data,
    Pixmap *p, Pixmap *s,
    XpmAttributes *attr, int flop);

extern void xpm_set_color(char **data,
    char ***out, int *lines, const char *color);

extern void xpm_destroy(char **data);

extern int xpmtobits(char *xpm[], unsigned char **bitsreturn,
    int *wreturn, int *hreturn, int *lreturn);

extern void xpm_print(char **xpm);
