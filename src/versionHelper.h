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

// VERSION is defined via the AC_INIT line in configure.ac

#ifdef HAVE_CONFIG_H
    #include "config.h"
#else
    #ifndef VERSION
        #define VERSION "unknown"
    #endif
    #ifndef PACKAGE_STRING
        #define PACKAGE_STRING "plasmastorm " VERSION
    #endif
#endif

#define VERSIONBY \
    "March 2024 by Mark Capella\n"
