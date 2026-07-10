from pathlib import Path

import imageio.v3 as iio
import numpy as np
from PIL import Image


Image.MAX_IMAGE_PIXELS = None


def read_image(path: Path) -> np.ndarray:
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")
    return iio.imread(path)


def find_interesting_crop_position(
    image: np.ndarray,
    crop_size: int = 2048,
    step: int = 1024
) -> tuple[int, int, int, int]:
    """
    Finds a crop with visible structure.

    Instead of taking the center crop, this scans the image and chooses
    the crop with the highest score based on non-zero pixel intensity.

    Returns:
        best_x, best_y, crop_width, crop_height
    """
    height, width = image.shape[:2]

    crop_height = min(crop_size, height)
    crop_width = min(crop_size, width)

    best_score = -1.0
    best_x = 0
    best_y = 0

    for y in range(0, height - crop_height + 1, step):
        for x in range(0, width - crop_width + 1, step):
            crop = image[y:y + crop_height, x:x + crop_width]

            # Ignore pure black pixels as much as possible
            nonzero = crop[crop > 0]

            if nonzero.size == 0:
                score = 0.0
            else:
                score = float(np.mean(nonzero)) + float(np.percentile(nonzero, 99))

            if score > best_score:
                best_score = score
                best_x = x
                best_y = y

    print("Selected crop:")
    print(f"  full image size: width={width}, height={height}")
    print(f"  crop size:       width={crop_width}, height={crop_height}")
    print(f"  x range:         {best_x} to {best_x + crop_width}")
    print(f"  y range:         {best_y} to {best_y + crop_height}")
    print(f"  crop score:      {best_score:.3f}")

    return best_x, best_y, crop_width, crop_height


def crop_image(
    image: np.ndarray,
    x: int,
    y: int,
    crop_width: int,
    crop_height: int
) -> np.ndarray:
    """
    Crops the image using explicit crop coordinates.
    """
    return image[y:y + crop_height, x:x + crop_width]


def stretch_for_display(image: np.ndarray) -> np.ndarray:
    """
    Contrast stretch for visualization only.

    This does not affect the benchmark data.
    It only makes the PNG preview easier to see.
    """
    image = image.astype(np.float32)

    nonzero = image[image > 0]

    if nonzero.size > 0:
        low, high = np.percentile(nonzero, [1, 99])
    else:
        low, high = np.percentile(image, [1, 99])

    if high <= low:
        high = low + 1.0

    image = (image - low) / (high - low)
    image = np.clip(image, 0, 1)
    image = image * 255.0

    return image.astype(np.uint8)


def save_image(path: Path, image: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    iio.imwrite(path, image)
    print(f"Saved {path}")


def main() -> None:
    input_pgm = Path("output/input.pgm")
    gaussian_pgm = Path("output/gaussian_seq.pgm")

    results_dir = Path("results")
    crop_size = 2048
    step = 1024

    original = read_image(input_pgm)
    gaussian = read_image(gaussian_pgm)

    # Find crop position on the original image
    crop_x, crop_y, crop_width, crop_height = find_interesting_crop_position(
        original,
        crop_size=crop_size,
        step=step
    )

    # Use exactly the same crop coordinates for original and Gaussian image
    original_crop_raw = crop_image(
        original,
        crop_x,
        crop_y,
        crop_width,
        crop_height
    )

    gaussian_crop_raw = crop_image(
        gaussian,
        crop_x,
        crop_y,
        crop_width,
        crop_height
    )

    # Contrast-stretch only for visualization
    original_crop = stretch_for_display(original_crop_raw)
    gaussian_crop = stretch_for_display(gaussian_crop_raw)

    save_image(results_dir / "original_crop.png", original_crop)
    save_image(results_dir / "gaussian_crop.png", gaussian_crop)

    print("\nRaw crop statistics:")
    for name, img in [("original", original_crop_raw), ("gaussian", gaussian_crop_raw)]:
        print(name)
        print("  min:", img.min())
        print("  max:", img.max())
        print("  mean:", img.mean())
        print("  percentiles:", np.percentile(img, [0, 50, 90, 95, 99, 99.5, 100]))


if __name__ == "__main__":
    main()