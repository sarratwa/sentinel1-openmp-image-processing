from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def main() -> None:
    csv_path = Path("results/benchmark_results.csv")
    results_dir = Path("results")

    if not csv_path.exists():
        print(f"Error: CSV file not found: {csv_path}")
        print("Run the C program first to generate benchmark_results.csv")
        return

    results_dir.mkdir(parents=True, exist_ok=True)

    df = pd.read_csv(csv_path)

    required_columns = {
        "filter",
        "version",
        "threads",
        "runtime",
        "speedup",
        "efficiency",
    }

    missing_columns = required_columns.difference(df.columns)

    if missing_columns:
        print(
            "Error: CSV is missing these columns:",
            ", ".join(sorted(missing_columns)),
        )
        return

    print("Loaded benchmark results:")
    print(df)

    openmp_df = df[df["version"] == "openmp"].copy()

    if openmp_df.empty:
        print("Error: No OpenMP rows found in CSV.")
        return

    openmp_df = openmp_df.sort_values("threads")

    # Runtime versus threads
    plt.figure()
    plt.plot(
        openmp_df["threads"],
        openmp_df["runtime"],
        marker="o",
    )
    plt.xlabel("Number of Threads")
    plt.ylabel("Runtime in Seconds")
    plt.title("OpenMP Gaussian Filter: Runtime vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.tight_layout()
    plt.savefig(
        results_dir / "runtime_vs_threads.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.close()

    # Speedup versus threads
    plt.figure()
    plt.plot(
        openmp_df["threads"],
        openmp_df["speedup"],
        marker="o",
    )
    plt.xlabel("Number of Threads")
    plt.ylabel("Speedup")
    plt.title("OpenMP Gaussian Filter: Speedup vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.tight_layout()
    plt.savefig(
        results_dir / "speedup_vs_threads.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.close()

    # Efficiency versus threads
    plt.figure()
    plt.plot(
        openmp_df["threads"],
        openmp_df["efficiency"],
        marker="o",
    )
    plt.xlabel("Number of Threads")
    plt.ylabel("Efficiency")
    plt.title("OpenMP Gaussian Filter: Efficiency vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.tight_layout()
    plt.savefig(
        results_dir / "efficiency_vs_threads.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.close()

    print("\nPlots saved:")
    print(results_dir / "runtime_vs_threads.png")
    print(results_dir / "speedup_vs_threads.png")
    print(results_dir / "efficiency_vs_threads.png")


if __name__ == "__main__":
    main()