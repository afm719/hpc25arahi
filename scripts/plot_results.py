# ==============================================================================
# plot_results_simple.py
#
# A simplified Python script to generate Speedup and Efficiency plots.
# The filenames are hardcoded inside the script.
#
# Usage:
# python plot_results.py
# (No arguments needed)
# ==============================================================================

import pandas as pd
import matplotlib.pyplot as plt
import os

def generate_plots(csv_file_path, x_axis_column):
    """
    Reads a CSV file, calculates speedup and efficiency, and generates plots.
    """
    try:
        df = pd.read_csv(csv_file_path)
    except FileNotFoundError:
        print(f"--> WARNING: The file '{csv_file_path}' was not found. Skipping.")
        return

    # Remove any rows with errors before processing
    df = df[~df.isin(['ERROR', 'ERROR_TIME_NOT_FOUND']).any(axis=1)]
    # Convert columns to numeric, errors='coerce' will turn non-numbers into NaN
    df['Total_Time'] = pd.to_numeric(df['Total_Time'], errors='coerce')
    df = df.dropna()

    if df.empty:
        print(f"--> WARNING: No valid data found in '{csv_file_path}' after cleaning. Skipping.")
        return

    # Sort the data by the resource count for correct plotting
    df = df.sort_values(by=x_axis_column).reset_index(drop=True)
    
    print(f"\nProcessing data from: {csv_file_path}")
    print(df)
    
    # --- Calculate Speedup and Efficiency ---
    baseline_time = df['Total_Time'].iloc[0]
    df['Speedup'] = baseline_time / df['Total_Time']
    df['Efficiency'] = df['Speedup'] / df[x_axis_column]

    # --- Generate Speedup Plot ---
    plt.figure(figsize=(10, 6))
    plt.plot(df[x_axis_column], df['Speedup'], 'o-', label='Speedup Real', color='blue')
    plt.plot(df[x_axis_column], df[x_axis_column], '--', label='Speedup Ideal', color='red')
    
    plt.title(f'Speedup vs. Number of {x_axis_column}')
    plt.xlabel(f'Number of {x_axis_column}')
    plt.ylabel('Speedup (Acceleration)')
    plt.grid(True, which='both', linestyle='--')
    plt.legend()
    plt.xticks(df[x_axis_column])
    
    base_name = os.path.splitext(os.path.basename(csv_file_path))[0]
    speedup_plot_path = f"plots/{base_name}_speedup.png"
    plt.savefig(speedup_plot_path)
    print(f"Speedup plot saved to: {speedup_plot_path}")
    plt.close()

    # --- Generate Efficiency Plot ---
    plt.figure(figsize=(10, 6))
    plt.plot(df[x_axis_column], df['Efficiency'] * 100, 'o-', label='Efficiency Real', color='green')
    plt.axhline(y=100, color='red', linestyle='--', label='Ideal Efficiency (100%)')

    plt.title(f'Efficiency vs. Number of {x_axis_column}')
    plt.xlabel(f'Number of {x_axis_column}')
    plt.ylabel('Parallel Efficiency (%)')
    plt.ylim(0, 110)
    plt.grid(True, which='both', linestyle='--')
    plt.legend()
    plt.xticks(df[x_axis_column])

    efficiency_plot_path = f"plots/{base_name}_efficiency.png"
    plt.savefig(efficiency_plot_path)
    print(f"Efficiency plot saved to: {efficiency_plot_path}")
    plt.close()

if __name__ == "__main__":
    # --- List of files to process ---
    # The script will loop through these files automatically.
    # Just add or remove files from this dictionary.
    files_to_plot = {
        "plots/openmp_scaling_metrics.csv": "Threads",
        "plots/strong_scaling_results.csv": "Nodes",
        "plots/weak_scaling_results.csv": "Nodes"
    }

    print("Starting plot generation...")
    for csv_path, x_column in files_to_plot.items():
        generate_plots(csv_path, x_column)
    
    print("\nAll plot generation tasks are complete!")