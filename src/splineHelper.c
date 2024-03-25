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
 * */
#define GSL_INTERP_MESSAGE
#include <pthread.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include "splineHelper.h"


void spline_interpol(const double* px, const double* py, int np,
    const double* x, double* y, int nx) {

    gsl_spline* spline =
        gsl_spline_alloc(SPLINE_INTERP, np);
    gsl_spline_init(spline, px, py, np);

    gsl_interp_accel* acc = gsl_interp_accel_alloc();
    for (int i = 0; i < nx; i++) {
        y[i] = gsl_spline_eval(spline, x[i], acc);
    }
    gsl_interp_accel_free(acc);

    gsl_spline_free(spline);
}
