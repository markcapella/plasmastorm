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

#include "pixmaps.h"

#include "generatedShapeIncludes.h"

#include "plasmastorm.h"
#include "Storm.h"

#include "Pixmaps/plasmastorm.xpm"
XPM_TYPE** mPlasmastormLogoPixmap =
	(XPM_TYPE**) plasmastorm_xpm;

// StormShape0.xpm stubs internally to StormShape0_xpm var name.
#define STORM_SHAPE_FILENAME(x) StormShape##x##_xpm,
XPM_TYPE** mResourcesShapes[] =
	{ALL_STORM_FILENAMES NULL};
#undef STORM_SHAPE_FILENAME

