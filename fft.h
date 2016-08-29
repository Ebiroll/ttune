#ifndef FFT_H
#define FFT_H

//  KAISER_WINDOW
enum {HANNING_WINDOW = 0, BLACKMAN_WINDOW, GAUSSIAN_WINDOW, WELCH_WINDOW, BARTLETT_WINDOW, RECTANGULAR_WINDOW, HAMMING_WINDOW};

#ifdef __cplusplus
extern "C" {
#endif

void compute_window(double *window,int window_type,int N);

#ifdef __cplusplus
}
#endif


#endif // FFT_H

