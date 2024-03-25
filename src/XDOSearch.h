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

int doXDOWindowSearch(const xdo_t* xdo, const xdo_search_t* search,
    Window** windowlist_ret, unsigned int* nwindows_ret);

int checkXDOWindowMatch(const xdo_t* xdo,
    Window window, const xdo_search_t* search);

int initXDOSearchRegex(const char *pattern, regex_t *re);

void findXDOWindowMatch(const xdo_t* xdo, Window window,
    const xdo_search_t* search, Window** windowlist_ret,
    unsigned int* nwindows_ret, unsigned int* windowlist_size,
    int current_depth);
