#ifndef FILTERS_H
#define FILTERS_H

#include "image_io.h"

/*
    Gaussian filter versions.

    The sequential version is the baseline.
    The OpenMP version parallelizes the outer image loop.
*/
void gaussian_filter_sequential(Image input, Image output);
void gaussian_filter_openmp(Image input, Image output);

#endif