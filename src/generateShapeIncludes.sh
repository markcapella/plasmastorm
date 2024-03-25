#!/bin/sh
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
root="${1:-..}"

# Generate Storm related pixmap glue.
out="generatedShapeIncludes.h"
echo "#pragma once" > "$out"
echo "/* -""copyright-" >> "$out"
echo "*/" >> "$out"

ls "$root/src/Pixmaps"/StormShape*.xpm | sed "s/^/#include \"/;s/$/\"/" >> "$out"

echo "#define ALL_STORM_FILENAMES \\" >> "$out"
for i in $(seq `ls "$root/src/Pixmaps"/StormShape*.xpm | wc -l`) ; do \
   printf 'STORM_SHAPE_FILENAME(%d) \\\n' `expr $i - 1` ; \
   done >> "$out"
echo >> "$out"

if [ -x "$root/addCopyright.sh" ] ; \
   then "$root/addCopyright.sh" "$out"; \
fi
