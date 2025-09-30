# ==============================================================================
# plot_advanced.py
#
# A Python script to generate advanced performance plots:
# 1. A stacked bar chart of time breakdown (Compute, Comm, Wait).
# 2. A throughput plot (e.g., in Million Cells / Second).
#
# Usage:
# python plot_advanced.py <path_to_csv_file> <x_axis_column>
# ==============================================================================

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def generate_advanced_plots(csv_file_path, x_axis_column):
    """
    Reads a CSV file and generates advanced performance plots.
    """
    try:
        df = pd.read_csv(csv_file_path)
    except FileNotFoundError:
        print(f"--> WARNING: The file '{csv_file_path}' was not found. Skipping.")
        return

    # --- 1. Data Cleaning and Preparation ---
    df_clean = df.dropna()
    df_clean = df_clean[~df_clean.isin(['ERROR', 'ERROR_TIME_NOT_FOUND']).any(axis=1)]
    
    time_cols = ['Total_Time', 'Comm_Time', 'Compute_Time', 'Wait_Time']
    for col in time_cols:
        df_clean[col] = pd.to_numeric(df_clean[col], errors='coerce')

    df_clean = df_clean.dropna()
    
    if df_clean.empty:
        print(f"--> WARNING: No valid data in '{csv_file_path}' for advanced plots. Skipping.")
        return

    df_clean = df_clean.sort_values(by=x_axis_column).reset_index(drop=True)
    print(f"\nProcessing advanced plots for: {csv_file_path}")

    # --- 2. Generate Time Breakdown Stacked Bar Chart ---
    plt.figure(figsize=(12, 7))
    
    # Bottom of each bar starts at 0
    bottom = pd.Series([0] * len(df_clean))
    
    # Plot each component
    plt.bar(df_clean[x_axis_column], df_clean['Compute_Time'], bottom=bottom, label='Compute Time')
    bottom += df_clean['Compute_Time']
    
    plt.bar(df_clean[x_axis_column], df_clean['Comm_Time'], bottom=bottom, label='Communication Time')
    bottom += df_clean['Comm_Time']
    
    plt.bar(df_clean[x_axis_column], df_clean['Wait_Time'], bottom=bottom, label='Wait Time')

    plt.title(f'Time Breakdown vs. Number of {x_axis_column}')
    plt.xlabel(f'Number of {x_axis_column}')
    plt.ylabel('Time (seconds)')
    plt.legend()
    plt.grid(axis='y', linestyle='--')
    plt.xticks(df_clean[x_axis_column])
    
    base_name = os.path.splitext(os.path.basename(csv_file_path))[0]
    breakdown_plot_path = f"plots/{base_name}_time_breakdown.png"
    plt.savefig(breakdown_plot_path)
    print(f"Time breakdown plot saved to: {breakdown_plot_path}")
    plt.close()

    # --- 3. Generate Throughput Plot ---
    # Throughput requires problem size information
    if 'Size_X' in df_clean.columns and 'Size_Y' in df_clean.columns:
        # Calculate throughput in Million Cells per Second
        df_clean['Throughput'] = (df_clean['Size_X'] * df_clean['Size_Y']) / df_clean['Total_Time'] / 1e6

        plt.figure(figsize=(10, 6))
        plt.plot(df_clean[x_axis_column], df_clean['Throughput'], 'o-', label='Throughput', color='orange')
        
        plt.title(f'Throughput vs. Number of {x_axis_column}')
        plt.xlabel(f'Number of {x_axis_column}')
        plt.ylabel('Throughput (Million Cells / Second)')
        plt.grid(True, which='both', linestyle='--')
        plt.legend()
        plt.xticks(df_clean[x_axis_column])

        throughput_plot_path = f"plots/{base_name}_throughput.png"
        plt.savefig(throughput_plot_path)
        print(f"Throughput plot saved to: {throughput_plot_path}")
        plt.close()
    else:
        print(f"--> INFO: 'Size_X'/'Size_Y' not found in '{csv_file_path}'. Skipping throughput plot.")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python plot_advanced.py <path_to_csv_file> <x_axis_column>")
        sys.exit(1)
        
    csv_path = sys.argv[1]
    x_column = sys.argv[2]
    generate_advanced_plots(csv_path, x_column)