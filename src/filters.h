#ifndef FILTERS_H
#define FILTERS_H

#include "image_io.h"

/*
    A square convolution kernel.

    weights is a flattened size*size array of integer weights
    (row-major). sum is the normalization divisor (sum of all weights).
*/
typedef struct {
    int size;     /* must be odd: 3, 5, 7, ... */
    float sum;
    int *weights; /* owned by this struct, free with free_kernel() */
} Kernel;

/*
    Generates a discrete binomial-approximation Gaussian kernel of the
    given odd size using Pascal's triangle. This is the standard way
    to build integer Gaussian-like kernels: the 1D binomial coefficients
    of row (size - 1) converge to a Gaussian by the de Moivre-Laplace
    theorem, and the 2D kernel is their outer product.

    size=3 reproduces the original hand-written kernel:
        1 2 1
        2 4 2
        1 2 1
    exactly, so existing 3x3 results are unaffected.
*/
Kernel generate_binomial_kernel(int size);

/* Frees the weights array owned by the kernel. */
void free_kernel(Kernel kernel);

void gaussian_filter_sequential(Image input, Image output, Kernel kernel);
void gaussian_filter_openmp(Image input, Image output, Kernel kernel);

/*
    Lee filter parameters.

    window_size: size of the square local-statistics window (odd: 3, 5, 7, ...)

    noise_variance: the assumed noise variance Cu^2 = 1 / ENL, where ENL is
    the equivalent number of looks. Sentinel-1 IW GRDH products are commonly
    documented with ENL around 4.9 due to multi-looking during GRD processing,
    which is the default used here (Cu^2 ~= 0.204). This is a nominal,
    standard reference value, not a value calibrated for this specific
    product/scene -- see the README limitations section for why exact
    calibration isn't attempted here.
*/
typedef struct {
    int window_size;
    float noise_variance;
} LeeFilterParams;

void lee_filter_sequential(Image input, Image output, LeeFilterParams params);
void lee_filter_openmp(Image input, Image output, LeeFilterParams params);

#endif