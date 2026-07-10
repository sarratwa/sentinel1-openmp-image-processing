from pathlib import Path
import imageio.v3 as iio
import numpy as np

from PIL import Image
Image.MAX_IMAGE_PIXELS = None

for file in [
    "output/input.pgm",
    "output/gaussian_seq.pgm",
    "output/gaussian_omp.pgm",
]:
    img = iio.imread(Path(file))
    print(file)
    print("  shape:", img.shape)
    print("  dtype:", img.dtype)
    print("  min:", img.min())
    print("  max:", img.max())
    print("  mean:", img.mean())
    print("  percentiles:", np.percentile(img, [0, 1, 5, 50, 95, 99, 100]))