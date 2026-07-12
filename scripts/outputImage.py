from pathlib import Path

import imageio.v3 as iio
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image


Image.MAX_IMAGE_PIXELS = None


def print_diff_statistics(
    name: str,
    original: np.ndarray,
    filtered: np.ndarray,
) -> None:
    """
    Prints numeric proof of what a filter actually changed, independent
    of how the images get displayed. This matters because a filter can
    be working correctly while still being nearly invisible in a
    downscaled thumbnail (browser-rendered PNGs at a fraction of their
    native resolution erase single-pixel differences through
    anti-aliasing) or in a heavily textured crop (an adaptive speckle
    filter is *designed* to leave strong texture/edges alone).
    """
    original_float = original.astype(np.float64)
    filtered_float = filtered.astype(np.float64)

    diff = np.abs(original_float - filtered_float)

    changed_fraction = (diff > 1e-6).mean()

    print(f"\n{name} vs original -- pixel-level difference:")
    print(f"  Mean absolute difference:   {diff.mean():.4f}")
    print(f"  Max absolute difference:    {diff.max():.4f}")
    print(f"  Fraction of pixels changed: {changed_fraction:.1%}")

    if diff.mean() < 1e-6:
        print("  WARNING: filter appears to have made no change at all to this crop.")


def create_difference_image(
    original: np.ndarray,
    filtered: np.ndarray,
) -> np.ndarray:
    """
    Builds a contrast-stretched visualization of |original - filtered|.

    This directly visualizes what a filter removed, which stays visible
    even when the filtered image looks nearly identical to the original
    at thumbnail scale -- the removed noise itself becomes the image.
    """
    diff = np.abs(
        original.astype(np.float32) - filtered.astype(np.float32)
    )

    low, high = np.percentile(diff, [1, 99.5])

    if high <= low:
        high = low + 1.0

    normalized = (diff - low) / (high - low)
    normalized = np.clip(normalized, 0.0, 1.0)

    return (normalized * 255.0).astype(np.uint8)


def save_difference_maps(
    path: Path,
    gaussian_diff: np.ndarray,
    lee_diff: np.ndarray,
) -> None:
    """
    Saves a side-by-side figure of what each filter actually removed.
    """
    path.parent.mkdir(parents=True, exist_ok=True)

    figure, axes = plt.subplots(1, 2, figsize=(12, 6))

    axes[0].imshow(gaussian_diff, cmap="inferno")
    axes[0].set_title("Removed by Gaussian Filter\n(|original - filtered|)")
    axes[0].axis("off")

    axes[1].imshow(lee_diff, cmap="inferno")
    axes[1].set_title("Removed by Lee Filter\n(|original - filtered|)")
    axes[1].axis("off")

    figure.tight_layout()
    figure.savefig(path, dpi=300, bbox_inches="tight")
    plt.close(figure)

    print(f"Saved: {path}")


def extract_zoom_region(
    crop: np.ndarray,
    zoom_size: int = 300,
) -> np.ndarray:
    """
    Extracts a small region from the center of a larger crop, for
    genuinely close pixel-level inspection.

    A 2048x2048 preview PNG typically gets rendered by a browser at a
    few hundred pixels wide inside a README table, which erases
    single-pixel speckle differences through downsampling. A much
    smaller crop, displayed at or near its native size, does not have
    this problem.
    """
    height, width = crop.shape[:2]

    zoom_height = min(zoom_size, height)
    zoom_width = min(zoom_size, width)

    start_y = (height - zoom_height) // 2
    start_x = (width - zoom_width) // 2

    return crop[
        start_y:start_y + zoom_height,
        start_x:start_x + zoom_width,
    ]


def save_zoom_comparison(
    path: Path,
    original: np.ndarray,
    gaussian: np.ndarray,
    lee: np.ndarray,
) -> None:
    """
    Same idea as save_comparison(), but for a small, tightly zoomed
    region, displayed with nearest-neighbor upscaling so individual
    pixels are actually visible rather than blurred by resizing.
    """
    path.parent.mkdir(parents=True, exist_ok=True)

    figure, axes = plt.subplots(1, 3, figsize=(15, 5))

    for axis, image, title in zip(
        axes,
        [original, gaussian, lee],
        ["Original (zoom)", "Gaussian Filter (zoom)", "Lee Filter (zoom)"],
    ):
        axis.imshow(image, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
        axis.set_title(title)
        axis.axis("off")

    figure.tight_layout()
    figure.savefig(path, dpi=300, bbox_inches="tight")
    plt.close(figure)

    print(f"Saved: {path}")


def find_original_tiff(data_dir: Path) -> Path:
    """
    Finds a TIFF file inside the data directory.

    This avoids having to type the very long Sentinel-1 filename
    directly into this script.
    """
    candidates = sorted(
        list(data_dir.glob("*.tif"))
        + list(data_dir.glob("*.tiff"))
        + list(data_dir.glob("*.TIF"))
        + list(data_dir.glob("*.TIFF"))
    )

    if not candidates:
        raise FileNotFoundError(
            f"No TIFF file found inside: {data_dir}"
        )

    if len(candidates) > 1:
        print("Multiple TIFF files found:")

        for index, path in enumerate(candidates, start=1):
            print(f"  {index}: {path}")

        print(f"\nUsing the first TIFF: {candidates[0]}")

    return candidates[0]


def read_image(path: Path) -> np.ndarray:
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    print(f"Reading: {path}")

    image = iio.imread(path)

    # Some TIFF readers may return a shape such as (1, height, width).
    if image.ndim == 3 and image.shape[0] == 1:
        image = image[0]

    # If a multi-band image is returned, use its first raster band.
    elif image.ndim == 3:
        image = image[..., 0]

    if image.ndim != 2:
        raise ValueError(
            f"Expected a two-dimensional grayscale raster, "
            f"but received shape {image.shape} from {path}"
        )

    return image


def find_valid_data_bounding_box(
    image: np.ndarray,
    row_col_valid_threshold: float = 0.5,
) -> tuple[int, int, int, int]:
    """
    Trims the image down to the region that actually contains data,
    excluding large no-data borders common at Sentinel-1 swath edges.

    A row/column is considered "valid" if at least
    row_col_valid_threshold of its pixels are finite and > 0.
    The returned box is the smallest rectangle enclosing all valid
    rows and columns from the outside in.

    This does not guarantee every pixel inside the box is valid data
    (some no-data patches can still exist deeper inside the swath) --
    it only removes the large solid no-data borders. The crop search
    below adds a second, stricter check on top of this.
    """
    finite = np.isfinite(image)
    valid_mask = finite & (image > 0)

    row_valid_ratio = valid_mask.mean(axis=1)
    col_valid_ratio = valid_mask.mean(axis=0)

    valid_rows = np.where(row_valid_ratio >= row_col_valid_threshold)[0]
    valid_cols = np.where(col_valid_ratio >= row_col_valid_threshold)[0]

    if valid_rows.size == 0 or valid_cols.size == 0:
        print(
            "Warning: could not find a confident valid-data region; "
            "using the full image instead."
        )
        return 0, image.shape[0], 0, image.shape[1]

    row_start, row_end = int(valid_rows[0]), int(valid_rows[-1]) + 1
    col_start, col_end = int(valid_cols[0]), int(valid_cols[-1]) + 1

    print("Valid-data bounding box (excludes no-data swath borders):")
    print(f"  rows: {row_start} to {row_end}")
    print(f"  cols: {col_start} to {col_end}")

    return row_start, row_end, col_start, col_end


def find_interesting_crop_position(
    image: np.ndarray,
    crop_size: int = 2048,
    step: int = 1024,
    min_valid_ratio: float = 0.98,
) -> tuple[int, int, int, int]:
    """
    Finds a crop containing visible texture and contrast.

    The crop position is calculated only from the original image.
    The exact same coordinates are then used for the filtered image.

    min_valid_ratio is intentionally strict (98%, not the old 5%):
    a crop with a large no-data (solid black) region can still pass
    a lax ratio check while looking nothing like a real SAR scene.
    Searching is also restricted to the valid-data bounding box, so
    swath-edge no-data borders are excluded from consideration
    entirely rather than merely down-weighted.
    """
    box_row_start, box_row_end, box_col_start, box_col_end = (
        find_valid_data_bounding_box(image)
    )

    height, width = image.shape

    crop_height = min(crop_size, box_row_end - box_row_start)
    crop_width = min(crop_size, box_col_end - box_col_start)

    max_y = max(box_row_start, box_row_end - crop_height)
    max_x = max(box_col_start, box_col_end - crop_width)

    y_positions = list(range(box_row_start, max_y + 1, step))
    x_positions = list(range(box_col_start, max_x + 1, step))

    if not y_positions or y_positions[-1] != max_y:
        y_positions.append(max_y)

    if not x_positions or x_positions[-1] != max_x:
        x_positions.append(max_x)

    best_score = float("-inf")
    best_x = box_col_start
    best_y = box_row_start
    found_valid_crop = False

    for y in y_positions:
        for x in x_positions:
            crop = image[
                y:y + crop_height,
                x:x + crop_width,
            ].astype(np.float32)

            finite_pixels = crop[np.isfinite(crop)]
            nonzero = finite_pixels[finite_pixels > 0]

            nonzero_ratio = nonzero.size / crop.size

            # Reject any crop that isn't almost entirely valid data,
            # instead of merely down-weighting no-data pixels.
            if nonzero_ratio < min_valid_ratio:
                continue

            found_valid_crop = True

            p50 = np.percentile(nonzero, 50)
            p95 = np.percentile(nonzero, 95)
            p99 = np.percentile(nonzero, 99)
            standard_deviation = np.std(nonzero)

            contrast_score = p95 - p50
            texture_score = standard_deviation

            score = (
                contrast_score
                + texture_score
                + 0.1 * p99
            )

            if score > best_score:
                best_score = score
                best_x = x
                best_y = y

    if not found_valid_crop:
        print(
            f"Warning: no crop met the {min_valid_ratio:.0%} valid-data "
            "threshold; falling back to the first searched position. "
            "The preview image may include some no-data area."
        )

    print("\nSelected crop:")
    print(f"  Full image size: {width} x {height}")
    print(f"  Crop size:       {crop_width} x {crop_height}")
    print(f"  X range:         {best_x} to {best_x + crop_width}")
    print(f"  Y range:         {best_y} to {best_y + crop_height}")
    print(f"  Crop score:      {best_score:.3f}")

    return best_x, best_y, crop_width, crop_height


def crop_image(
    image: np.ndarray,
    x: int,
    y: int,
    crop_width: int,
    crop_height: int,
) -> np.ndarray:
    return image[
        y:y + crop_height,
        x:x + crop_width,
    ]


def calculate_display_range(
    original_crop: np.ndarray,
) -> tuple[float, float]:
    """
    Calculates one display range from the original crop.

    The same range is applied to both original and Gaussian output.
    This makes their visual comparison fair.
    """
    original_float = original_crop.astype(np.float32)

    valid_pixels = original_float[
        np.isfinite(original_float) & (original_float > 0)
    ]

    if valid_pixels.size == 0:
        valid_pixels = original_float[np.isfinite(original_float)]

    if valid_pixels.size == 0:
        return 0.0, 1.0

    low, high = np.percentile(valid_pixels, [1, 99])

    if high <= low:
        high = low + 1.0

    return float(low), float(high)


def stretch_for_display(
    image: np.ndarray,
    low: float,
    high: float,
) -> np.ndarray:
    """
    Converts a raw TIFF crop into an 8-bit preview image.

    This changes only the PNG visualization. It does not modify
    the original TIFF or the filtered TIFF.
    """
    image_float = image.astype(np.float32)

    image_float = np.nan_to_num(
        image_float,
        nan=low,
        posinf=high,
        neginf=low,
    )

    normalized = (image_float - low) / (high - low)
    normalized = np.clip(normalized, 0.0, 1.0)

    return (normalized * 255.0).astype(np.uint8)


def save_png(path: Path, image: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    iio.imwrite(path, image)
    print(f"Saved: {path}")


def save_comparison(
    path: Path,
    original: np.ndarray,
    gaussian: np.ndarray,
    lee: np.ndarray,
) -> None:
    """
    Creates one side-by-side image for documentation or presentation.
    """
    path.parent.mkdir(parents=True, exist_ok=True)

    figure, axes = plt.subplots(1, 3, figsize=(18, 6))

    axes[0].imshow(original, cmap="gray", vmin=0, vmax=255)
    axes[0].set_title("Original Sentinel-1 TIFF")
    axes[0].axis("off")

    axes[1].imshow(gaussian, cmap="gray", vmin=0, vmax=255)
    axes[1].set_title("After Gaussian Filter")
    axes[1].axis("off")

    axes[2].imshow(lee, cmap="gray", vmin=0, vmax=255)
    axes[2].set_title("After Lee Filter")
    axes[2].axis("off")

    figure.tight_layout()
    figure.savefig(path, dpi=300, bbox_inches="tight")
    plt.close(figure)

    print(f"Saved: {path}")


def print_statistics(
    name: str,
    image: np.ndarray,
) -> None:
    finite_pixels = image[np.isfinite(image)]

    print(f"\n{name}:")
    print(f"  Shape: {image.shape}")
    print(f"  Type:  {image.dtype}")

    if finite_pixels.size == 0:
        print("  No finite pixel values found.")
        return

    print(f"  Min:   {finite_pixels.min()}")
    print(f"  Max:   {finite_pixels.max()}")
    print(f"  Mean:  {finite_pixels.mean():.3f}")
    print(
        "  Percentiles:",
        np.percentile(
            finite_pixels,
            [0, 50, 90, 95, 99, 99.5, 100],
        ),
    )


def main() -> None:
    data_dir = Path("data")
    output_dir = Path("output")
    results_dir = Path("results")

    original_tiff = find_original_tiff(data_dir)
    gaussian_tiff = output_dir / "gaussian_seq.tif"
    lee_tiff = output_dir / "lee_seq.tif"

    crop_size = 2048
    step = 1024

    original = read_image(original_tiff)
    gaussian = read_image(gaussian_tiff)

    if original.shape != gaussian.shape:
        raise ValueError(
            "Original and filtered images have different dimensions:\n"
            f"  Original: {original.shape}\n"
            f"  Gaussian: {gaussian.shape}"
        )

    have_lee = lee_tiff.exists()

    if have_lee:
        lee = read_image(lee_tiff)

        if original.shape != lee.shape:
            raise ValueError(
                "Original and Lee-filtered images have different dimensions:\n"
                f"  Original: {original.shape}\n"
                f"  Lee: {lee.shape}"
            )
    else:
        print(
            f"Warning: {lee_tiff} not found. Skipping the Lee filter panel "
            "(run main.exe first to generate it)."
        )
        lee = None

    print_statistics("Original full image", original)
    print_statistics("Gaussian full image", gaussian)

    if have_lee:
        print_statistics("Lee full image", lee)

    crop_x, crop_y, crop_width, crop_height = (
        find_interesting_crop_position(
            original,
            crop_size=crop_size,
            step=step,
        )
    )

    original_crop_raw = crop_image(
        original,
        crop_x,
        crop_y,
        crop_width,
        crop_height,
    )

    gaussian_crop_raw = crop_image(
        gaussian,
        crop_x,
        crop_y,
        crop_width,
        crop_height,
    )

    print_statistics("Original raw crop", original_crop_raw)
    print_statistics("Gaussian raw crop", gaussian_crop_raw)

    if have_lee:
        lee_crop_raw = crop_image(
            lee,
            crop_x,
            crop_y,
            crop_width,
            crop_height,
        )
        print_statistics("Lee raw crop", lee_crop_raw)

    # Use the same brightness range for all previews, so the comparison is fair.
    low, high = calculate_display_range(original_crop_raw)

    print("\nVisualization range:")
    print(f"  Low percentile value:  {low:.3f}")
    print(f"  High percentile value: {high:.3f}")

    original_crop_display = stretch_for_display(
        original_crop_raw,
        low,
        high,
    )

    gaussian_crop_display = stretch_for_display(
        gaussian_crop_raw,
        low,
        high,
    )

    save_png(
        results_dir / "original_crop.png",
        original_crop_display,
    )

    save_png(
        results_dir / "gaussian_crop.png",
        gaussian_crop_display,
    )

    if have_lee:
        lee_crop_display = stretch_for_display(
            lee_crop_raw,
            low,
            high,
        )

        save_png(
            results_dir / "lee_crop.png",
            lee_crop_display,
        )

        save_comparison(
            results_dir / "before_after_gaussian.png",
            original_crop_display,
            gaussian_crop_display,
            lee_crop_display,
        )

        # ------------------------------------------------------------
        # Numeric proof that the filters actually changed pixels,
        # independent of how visible that is in a downscaled thumbnail.
        # ------------------------------------------------------------
        print_diff_statistics("Gaussian", original_crop_raw, gaussian_crop_raw)
        print_diff_statistics("Lee", original_crop_raw, lee_crop_raw)

        # ------------------------------------------------------------
        # Difference maps: what each filter actually removed. Stays
        # visible even when the filtered image looks nearly identical
        # to the original at thumbnail scale.
        # ------------------------------------------------------------
        gaussian_diff = create_difference_image(original_crop_raw, gaussian_crop_raw)
        lee_diff = create_difference_image(original_crop_raw, lee_crop_raw)

        save_difference_maps(
            results_dir / "difference_maps.png",
            gaussian_diff,
            lee_diff,
        )

        # ------------------------------------------------------------
        # Zoomed-in crop: a 2048x2048 PNG gets rendered at a few hundred
        # pixels wide in a README table, which erases single-pixel
        # speckle differences through downsampling. This small region,
        # shown near its native size, does not have that problem.
        # ------------------------------------------------------------
        zoom_original = extract_zoom_region(original_crop_display)
        zoom_gaussian = extract_zoom_region(gaussian_crop_display)
        zoom_lee = extract_zoom_region(lee_crop_display)

        save_zoom_comparison(
            results_dir / "zoom_comparison.png",
            zoom_original,
            zoom_gaussian,
            zoom_lee,
        )
    else:
        print("Skipping the 3-panel comparison figure since the Lee output is missing.")

    print("\nVisualization completed successfully.")


if __name__ == "__main__":
    main()