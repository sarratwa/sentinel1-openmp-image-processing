#ifndef FILTERS_H
#define FILTERS_H

#include "image_io.h"

void gaussian_filter_sequential(Image input, Image output);
void gaussian_filter_openmp(Image input, Image output);

#endif