from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


def main():
    csv_path = Path("results/benchmark_results.csv")
    results_dir = Path("results")

    if not csv_path.exists():
        print(f"Error: CSV file not found: {csv_path}")
        print("Run the C program first to generate benchmark_results.csv")
        return

    results_dir.mkdir(parents=True, exist_ok=True)

    # Load benchmark results
    df = pd.read_csv(csv_path)

    print("Loaded benchmark results:")
    print(df)

    # Use only OpenMP rows for thread-based plots
    openmp_df = df[df["version"] == "openmp"].copy()

    if openmp_df.empty:
        print("Error: No OpenMP rows found in CSV.")
        return

    # Sort by thread count
    openmp_df = openmp_df.sort_values("threads")

    # ------------------------------------------------------------
    # Plot 1: Runtime vs Threads
    # ------------------------------------------------------------
    plt.figure()
    plt.plot(openmp_df["threads"], openmp_df["runtime"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Runtime in Seconds")
    plt.title("OpenMP Gaussian Filter: Runtime vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.savefig(results_dir / "runtime_vs_threads.png", dpi=300, bbox_inches="tight")
    plt.close()

    # ------------------------------------------------------------
    # Plot 2: Speedup vs Threads
    # ------------------------------------------------------------
    plt.figure()
    plt.plot(openmp_df["threads"], openmp_df["speedup"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Speedup")
    plt.title("OpenMP Gaussian Filter: Speedup vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.savefig(results_dir / "speedup_vs_threads.png", dpi=300, bbox_inches="tight")
    plt.close()

    # ------------------------------------------------------------
    # Plot 3: Efficiency vs Threads
    # ------------------------------------------------------------
    plt.figure()
    plt.plot(openmp_df["threads"], openmp_df["efficiency"], marker="o")
    plt.xlabel("Number of Threads")
    plt.ylabel("Efficiency")
    plt.title("OpenMP Gaussian Filter: Efficiency vs Threads")
    plt.grid(True)
    plt.xticks(openmp_df["threads"])
    plt.savefig(results_dir / "efficiency_vs_threads.png", dpi=300, bbox_inches="tight")
    plt.close()

    print("\nPlots saved:")
    print(results_dir / "runtime_vs_threads.png")
    print(results_dir / "speedup_vs_threads.png")
    print(results_dir / "efficiency_vs_threads.png")


if __name__ == "__main__":
    main()