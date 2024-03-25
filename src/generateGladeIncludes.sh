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
#
# create C code to get ui.xml in a string
#
# ISO C stipulates that the length of a string constant should
# not be larger than 4096, so we create a definition as in
# char mStringBuilder[] = {60,63,120,109,108,32,118,101,0};
#
root="${1:-..}"
in="ui.glade"
out="generatedGladeIncludes.h"

echo "/* This file is generated from '$in' by '$0' */" > "$out"
echo "/* -copyright-" >> "$out"
echo "*/" >> "$out"
echo "#pragma once" >> "$out"

echo "char mStringBuilder[] = {" >> "$out"

sed 's/^ *//' "$root/src/$in" | awk -v FS="" \
   'BEGIN{for(n=0;n<256;n++)ord[sprintf("%c",n)]=n;}
   {for (i=1;i<=NF;i++) printf "%d,", ord[$i];
      printf "%d,\n",ord["\n"];}' >> "$out"
rc1=$?
echo "0};">> "$out" 

if [ -x "$root/addCopyright.sh" ] ; \
   then "$root/addCopyright.sh" "$out" ; fi
rc2=$?
if [ "$rc1" -eq 0 ]; then
   exit "$rc2"
fi

exit $rc1
