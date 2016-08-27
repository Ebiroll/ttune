#ifndef TTUNEVG_H
#define TTUNEVG_H

#include <fftw3.h>
//#include <rfftw.h>

/*
 * The idea is to run fft in a separate thread and wait for the result with
 * pthread_join, we will receive the last holder index or 0

*/

//#define NFRAMES 12000
//#define NFRAMES 16384
#define NFRAMES 4096


#define MAX_T 4

struct fft_holder {

        int index;
        //unsigned char buffer[2 * NFRAMES + 3];                          /* Buffer data for input */

        int N_samples;

        double *windowed_input;
        fftw_complex *output;

        fftw_plan plan;

};

#endif // TTUNEVG_H

