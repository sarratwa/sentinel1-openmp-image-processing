#ifndef FILTERS_H
#define FILTERS_H

#include "image_io.h"

/*
    A square convolution kernel

    weights is a flattened size*size array of integer weights
    sum is the normalization divisor (sum of all weights)
*/
typedef struct {
    int size;     // we used 3x3 , 5x5 & 7x7
    float sum;
    int *weights; /* owned by this struct, free with free_kernel() */
} Kernel;

/*
    Creates a Gaussian-like smoothing kernel of the requested odd size.

    Pixels near the center receive larger weights, while pixels farther
    away receive smaller weights. This produces a smoothing effect similar
    to a Gaussian blur, while using simple integer weights.

    The kernel size must be odd so that it has one clear center pixel.

    https://stackoverflow.com/questions/39744615/gaussian-generated-kernel-and-given-in-the-book-are-not-same-why 
*/
Kernel generate_binomial_kernel(int size);

// Frees the weights array owned by the kernel
void free_kernel(Kernel kernel);

void gaussian_filter_sequential(Image input, Image output, Kernel kernel);
void gaussian_filter_openmp(Image input, Image output, Kernel kernel);

/*
    Settings used by the Lee speckle filter.

    window_size:
    The Lee filter does not use one fixed kernel like the Gaussian filter. 
    Instead, for every pixel, it examines a small square neighborhood around it and calculates the local mean and variance.

    &
    noise_variance:
    Sentinel-1 radar images contain speckle noise. 
    The Lee filter needs an estimate of how much variation is caused by that noise.

*/
typedef struct {
    int window_size;
    float noise_variance;
} LeeFilterParams;

void lee_filter_sequential(Image input, Image output, LeeFilterParams params);
void lee_filter_openmp(Image input, Image output, LeeFilterParams params);

#endif