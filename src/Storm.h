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

#include "plasmastorm.h"

#include <gtk/gtk.h>

extern void initStormModule();
int getResourcesShapeCount();
void createCombinedShapesList();
void createCombinedShapeSurfacesList();
void updateStormShapesAttributes();

extern void respondToStormsSettingsChanges();

void setStormItemSize();
void setStormItemSpeed();

extern void setStormItemState(StormItem*, float state);
void setStormItemsPerSecond();

int doCreateStormShapeEvent();
extern int doStallCreateStormShapeEvent();

extern StormItem* createStormItem(int);
int updateStormItem(StormItem*);

void createRandomStormShape(int w, int h, char***);

void eraseStormItem(StormItem*);

void setStormShapeColor(GdkRGBA);
char* getNextStormShapeColorAsString();
extern GdkRGBA getNextStormShapeColorAsRGB();

bool isStormItemFallen(StormItem*,
    int xPosition, int yPosition);
bool isStormItemBehindWindow(StormItem*,
    int xPosition, int yPosition);

void pushStormItemIntoItemset(StormItem*);
extern int drawAllStormItemsInItemset(cairo_t*);
extern int removeAllStormItemsInItemset();
void removeStormItemInItemset(StormItem*);

extern void logStormItem(StormItem*);
