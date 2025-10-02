import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

# --- CONFIGURATION ---
STRONG_CSV = "plots/strong_scaling_results.csv"
WEAK_CSV = "plots/weak_scaling_results.csv"
OPENMP_CSV = "plots/openmp_scaling_metrics.csv"
OUTPUT_DIR = "plots"
# --- END OF CONFIGURATION ---

def plot_time_breakdown(csv_file, x_axis_col, title, output_filename):
    """
    Generates a stacked bar chart showing the breakdown of execution time.
    """
    # 1. Load and prepare the data
    try:
        df = pd.read_csv(csv_file)
        print(f"Successfully loaded '{csv_file}'.")
    except FileNotFoundError:
        print(f"--- ERROR: The file '{csv_file}' was not found. ---")
        return

    # Clean and sort data
    df = df[~df.isin(['ERROR']).any(axis=1)]
    time_cols = ['Total_Time', 'Comm_Time', 'Compute_Time', 'Wait_Time']
    for col in [x_axis_col] + time_cols:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    df.dropna(inplace=True)
    df = df.sort_values(by=x_axis_col).reset_index(drop=True)

    if df.empty:
        print(f"--> WARNING: No valid data for '{csv_file}'. Skipping plot.")
        return

    # 2. Generate Line Plot
    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(12, 7))

    # X-axis values and labels
    x_values = df[x_axis_col]
    x_labels = [str(int(v)) for v in x_values]

    # Plotting each time component as a separate line
    ax.plot(x_labels, df['Total_Time'], 'o-', color='red', label='Total Time', markersize=8, linewidth=2.5, zorder=5)
    ax.plot(x_labels, df['Compute_Time'], 's--', color='forestgreen', label='Compute Time', markersize=6, zorder=4)
    
    # Only plot communication/wait times if they are significant (i.e., for MPI)
    if df['Comm_Time'].sum() > 0.01:
        ax.plot(x_labels, df['Comm_Time'], 'D--', color='dodgerblue', label='Communication Time', markersize=6, zorder=3)
        ax.plot(x_labels, df['Wait_Time'], 'x--', color='darkorange', label='Wait/Sync Time', markersize=7, zorder=3)

    # 3. Formatting the plot
    ax.set_title(title, fontsize=16, fontweight='bold')
    ax.set_xlabel(f'Number of {x_axis_col.replace("_", " ")}', fontsize=12)
    ax.set_ylabel('Time (seconds)', fontsize=12)
    ax.legend(fontsize=12)
    ax.grid(True, which='both', linestyle='--', axis='y', alpha=0.7)
    
    # Improve y-axis readability
    max_time = df['Total_Time'].max()
    ax.set_ylim(0, max_time * 1.15)

    # Add text labels for total time on the line plot
    for i, row in df.iterrows():
        ax.text(i, row['Total_Time'] + max_time * 0.02, f"{row['Total_Time']:.2f}s",
                ha='center', fontsize=10, color='black')

    # Save the plot
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    plot_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    print(f"Time breakdown plot saved to: '{plot_path}'")
    plt.close()


if __name__ == "__main__":
    print("\n--- Generating Time Breakdown Plots ---")

    # --- Strong Scaling Plot ---
    if os.path.exists(STRONG_CSV):
        plot_time_breakdown(
            csv_file=STRONG_CSV,
            x_axis_col='Nodes',
            title='Strong Scaling: Time Breakdown vs. Number of Nodes',
            output_filename='strong_scaling_time_breakdown.png'
        )
    else:
        print(f"\nSkipping Strong Scaling plot: '{STRONG_CSV}' not found.")

    # --- Weak Scaling Plot ---
    if os.path.exists(WEAK_CSV):
        plot_time_breakdown(
            csv_file=WEAK_CSV,
            x_axis_col='Nodes',
            title='Weak Scaling: Time Breakdown vs. Number of Nodes',
            output_filename='weak_scaling_time_breakdown.png'
        )
    else:
        print(f"\nSkipping Weak Scaling plot: '{WEAK_CSV}' not found.")

    # --- OpenMP Scaling Plot ---
    if os.path.exists(OPENMP_CSV):
        plot_time_breakdown(
            csv_file=OPENMP_CSV,
            x_axis_col='Threads',
            title='OpenMP Scaling: Time Breakdown vs. Number of Threads',
            output_filename='openmp_scaling_time_breakdown.png'
        )
    else:
        print(f"\nSkipping OpenMP Scaling plot: '{OPENMP_CSV}' not found.")

    print("\n--- All plot generation complete! ---")