import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Set a general style for the plots
plt.style.use('seaborn-v0_8-whitegrid')

def plot_openmp_scaling(filename="openmp_scaling_results.csv"):
    """Generates the speedup plot for OpenMP scaling."""
    df = pd.read_csv(filename)
    
    # Calculate Speedup
    time_1_thread = df.loc[df['Threads'] == 1, 'Total_Time'].iloc[0]
    df['Speedup'] = time_1_thread / df['Total_Time']
    
    plt.figure(figsize=(10, 6))
    plt.plot(df['Threads'], df['Speedup'], 'o-', label='Measured Speedup')
    plt.plot(df['Threads'], df['Threads'], '--', label='Ideal Speedup', color='gray')
    
    plt.title('OpenMP Scalability')
    plt.xlabel('Number of Threads')
    plt.ylabel('Speedup')
    plt.xscale('log', base=2)
    plt.yscale('log', base=2)
    plt.xticks(df['Threads'], df['Threads']) # Set all x-ticks
    plt.legend()
    plt.savefig('openmp_scaling.png')
    print(f"Plot saved to openmp_scaling.png")

def plot_strong_scaling(filename="strong_scaling_results.csv"):
    """Generates speedup and efficiency plots for Strong Scaling."""
    df = pd.read_csv(filename).sort_values(by='Nodes').reset_index()

    # Calculate Speedup
    time_1_node = df.loc[df['Nodes'] == 1, 'Total_Time'].iloc[0]
    df['Speedup'] = time_1_node / df['Total_Time']
    
    # Calculate Efficiency
    df['Efficiency'] = df['Speedup'] / df['Nodes']
    
    # Speedup Plot
    plt.figure(figsize=(10, 6))
    plt.plot(df['Nodes'], df['Speedup'], 'o-', label='Measured Speedup')
    plt.plot(df['Nodes'], df['Nodes'], '--', label='Ideal Speedup', color='gray')
    plt.title('Strong Scaling Speedup')
    plt.xlabel('Number of Nodes')
    plt.ylabel('Speedup')
    plt.xscale('log', base=2)
    plt.yscale('log', base=2)
    plt.xticks(df['Nodes'], df['Nodes'])
    plt.legend()
    plt.savefig('strong_scaling_speedup.png')
    print(f"Plot saved to strong_scaling_speedup.png")

    # Efficiency Plot
    plt.figure(figsize=(10, 6))
    plt.plot(df['Nodes'], df['Efficiency'] * 100, 'o-', label='Measured Efficiency')
    plt.axhline(100, linestyle='--', color='gray', label='Ideal Efficiency')
    plt.title('Strong Scaling Efficiency')
    plt.xlabel('Number of Nodes')
    plt.ylabel('Efficiency (%)')
    plt.xscale('log', base=2)
    plt.xticks(df['Nodes'], df['Nodes'])
    plt.ylim(0, 110)
    plt.legend()
    plt.savefig('strong_scaling_efficiency.png')
    print(f"Plot saved to strong_scaling_efficiency.png")

def plot_weak_scaling(filename="weak_scaling_results.csv"):
    """Generates the efficiency plot for Weak Scaling."""
    df = pd.read_csv(filename).sort_values(by='Nodes').reset_index()

    # Calculate Efficiency
    time_1_node = df.loc[df['Nodes'] == 1, 'Total_Time'].iloc[0]
    df['Efficiency'] = time_1_node / df['Total_Time']

    plt.figure(figsize=(10, 6))
    plt.plot(df['Nodes'], df['Efficiency'] * 100, 'o-', label='Measured Efficiency')
    plt.axhline(100, linestyle='--', color='gray', label='Ideal Efficiency')
    plt.title('Weak Scaling Efficiency')
    plt.xlabel('Number of Nodes')
    plt.ylabel('Efficiency (%)')
    plt.xscale('log', base=2)
    plt.xticks(df['Nodes'], df['Nodes'])
    plt.ylim(0, 110)
    plt.legend()
    plt.savefig('weak_scaling_efficiency.png')
    print(f"Plot saved to weak_scaling_efficiency.png")

if __name__ == "__main__":
    # Call the functions to generate all plots
    plot_openmp_scaling()
    plot_strong_scaling()
    plot_weak_scaling()