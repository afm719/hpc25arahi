import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

# --- CONFIGURATION ---
INPUT_FILENAME = "plots/weak_scaling_results.csv"
OUTPUT_DIR = "plots"
# --- END OF CONFIGURATION ---

def generate_weak_scaling_plots(csv_file_path, output_dir):
    """
    Reads weak scaling results, calculates scaled speedup and efficiency,
    and generates corresponding plots.
    """
    # 1. Load and prepare the data
    try:
        df = pd.read_csv(csv_file_path)
        print(f"Successfully loaded '{csv_file_path}'.")
    except FileNotFoundError:
        print(f"--- ERROR: The file '{csv_file_path}' was not found. ---")
        print("Please ensure the script is run from the project's root directory and that the results file exists.")
        sys.exit(1)

    # Clean and sort data
    df = df[~df.isin(['ERROR']).any(axis=1)]
    for col in ['Nodes', 'Total_Time']:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    df.dropna(inplace=True)
    df = df.sort_values(by='Nodes').reset_index(drop=True)

    if df.empty:
        print("--> WARNING: No valid data found after cleaning. Cannot generate plots.")
        return

    # 2. Calculate Weak Scaling Metrics
    # Baseline is the time taken on the smallest number of nodes (ideally 1)
    baseline_nodes = df['Nodes'].iloc[0]
    baseline_time = df['Total_Time'].iloc[0]

    if baseline_nodes != 1:
        print(f"--> WARNING: Baseline is calculated from {baseline_nodes} nodes, not 1. Results may be skewed.")

    # Scaled Speedup = N * (T_1 / T_N)
    # For weak scaling, ideal speedup is linear (equals the number of nodes)
    df['Scaled_Speedup'] = (baseline_time / df['Total_Time']) * df['Nodes']

    # Efficiency = T_1 / T_N
    # For weak scaling, ideal efficiency is 1.0 (or 100%)
    df['Efficiency'] = baseline_time / df['Total_Time']

    print("\n--- Calculated Metrics ---")
    print(df[['Nodes', 'Total_Time', 'Scaled_Speedup', 'Efficiency']])
    print("--------------------------\n")

    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)

    # 3. Generate Scaled Speedup Plot
    plt.style.use('seaborn-v0_8-whitegrid')
    plt.figure(figsize=(10, 6))

    plt.plot(df['Nodes'], df['Scaled_Speedup'], 'o-', label='Actual Scaled Speedup', color='dodgerblue', markersize=8)
    plt.plot(df['Nodes'], df['Nodes'], '--', label='Ideal Linear Speedup', color='red')

    plt.title('Weak Scaling: Scaled Speedup Analysis', fontsize=16, fontweight='bold')
    plt.xlabel('Number of Nodes', fontsize=12)
    plt.ylabel('Scaled Speedup', fontsize=12)
    plt.legend(fontsize=12)
    plt.grid(True, which='both', linestyle='--', alpha=0.7)
    plt.xticks(df['Nodes'])
    plt.ylim(bottom=0)

    speedup_plot_path = os.path.join(output_dir, "weak_scaling_speedup.png")
    plt.savefig(speedup_plot_path, dpi=150, bbox_inches='tight')
    print(f"Scaled Speedup plot saved to: '{speedup_plot_path}'")
    plt.close()

    # 4. Generate Efficiency Plot
    plt.figure(figsize=(10, 6))

    plt.plot(df['Nodes'], df['Efficiency'] * 100, 'o-', label='Actual Efficiency', color='forestgreen', markersize=8)
    plt.axhline(y=100, color='red', linestyle='--', label='Ideal Efficiency (100%)')

    plt.title('Weak Scaling: Parallel Efficiency', fontsize=16, fontweight='bold')
    plt.xlabel('Number of Nodes', fontsize=12)
    plt.ylabel('Efficiency', fontsize=12)
    plt.legend(fontsize=12)
    plt.grid(True, which='both', linestyle='--', alpha=0.7)
    plt.xticks(df['Nodes'])
    plt.ylim(0, 115) # Set y-axis from 0% to 115%

    # Add text labels for each point
    for i, row in df.iterrows():
        plt.text(row['Nodes'], row['Efficiency'] * 100 + 2, f"{row['Efficiency']*100:.1f}%",
                 ha='center', fontsize=10)

    efficiency_plot_path = os.path.join(output_dir, "weak_scaling_efficiency.png")
    plt.savefig(efficiency_plot_path, dpi=150, bbox_inches='tight')
    print(f"Efficiency plot saved to: '{efficiency_plot_path}'")
    plt.close()

if __name__ == "__main__":
    print("--- Generating Weak Scaling Analysis Plots ---")
    generate_weak_scaling_plots(INPUT_FILENAME, OUTPUT_DIR)
    print("\n--- Plot generation complete! ---")