/* fft.h
 *
 * Copyright (C) 2001-2008 Claudio Girardi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "fft.h"
#include <math.h>

void compute_window(double *window,int window_type,int N)
{
  int i;
  float t;
  float alpha = 1.0;
  float w_pwr;

  /* Calculate FFT Windowing function */
  for (i = 0; i < N; i++) {
    switch (window_type) {
    case HANNING_WINDOW:	/* Hanning */
      window[i] = 0.5 - 0.5 * cos(2.0 * M_PI * i / (N - 1.0));
      break;
    case BLACKMAN_WINDOW:	/* Blackman */
      window[i] = 0.42 - 0.5 * cos(2.0 * M_PI * i / (N - 1.0)) + 0.08 * cos(4.0 * M_PI * i / (N - 1.0));
      break;
    case GAUSSIAN_WINDOW:	/* Gaussian */
      alpha = 1.0;
      window[i] = exp(-alpha * (2.0 * i - N + 1.0) * (2.0 * i - N + 1.0) / ((N - 1.0) * (N - 1.0)));
      break;
    case WELCH_WINDOW:		/* Welch */
      window[i] = 1.0 - ((2.0 * i - N + 1.0) / (N - 1.0)) * ((2.0 * i - N + 1.0) / (N - 1.0));
      break;
    case BARTLETT_WINDOW:	/* Parzen or Bartlett (?) */
      window[i] = 1.0 - fabs((2.0 * i - N + 1.0) / (N - 1.0));
      break;
    case RECTANGULAR_WINDOW:	/* Rectangular */
      window[i] = 1.0;
      break;
    case HAMMING_WINDOW:	/* Hamming */
      window[i] = 0.54 - 0.46 * cos(2.0 * M_PI * i / (N - 1.0));
      break;
      /*
    case KAISER_WINDOW:	 //Kaiser
      t = (N - 1.0) / 2.0;
      alpha = 6.0 / t;		// from 4.0 / t to 9.0 / t
      window[i] = bessel_I0(alpha * sqrt(t * t - (i - t) * (i - t))) / bessel_I0(alpha * t);
      break;
      */
    default:
      window[i] = 1.0;
    }
  }
  /* normalize window to preserve power */
  //w_pwr = 0.0;
  //for (i = 0; i < N; i++) {
  //  w_pwr += window[i] * window[i];
  //}
  //for (i = 0; i < N; i++) {
  //  window[i] /= sqrt(w_pwr);
  //}
}
