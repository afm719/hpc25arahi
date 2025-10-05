import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from mpl_toolkits.mplot3d import Axes3D
import os

def parse_grid_data(filename):
    """
    Parses the log file and handles inconsistent grid sizes
    caused by race conditions in the MPI output.
    """
    grids = []
    if not os.path.exists(filename):
        print(f"❌ Error: Could not find the file at '{filename}'.")
        return grids

    with open(filename, 'r') as f:
        content = f.read()

    time_step_blocks = content.split("--- End of Full Grid Output ---")
    
    expected_shape = None
    initial_grid_count = 0

    for block in time_step_blocks:
        current_grid_lines = []
        lines_in_block = block.strip().split('\n')
        for line in lines_in_block:
            line = line.strip()
            if not line or line.startswith("[DEBUG]"):
                continue
            try:
                row_data = [float(x) for x in line.split()]
                if row_data:
                    current_grid_lines.append(row_data)
            except ValueError:
                continue
        
        if current_grid_lines and all(len(row) == len(current_grid_lines[0]) for row in current_grid_lines):
            grid = np.array(current_grid_lines)
            initial_grid_count += 1
            
            if expected_shape is None:
                expected_shape = grid.shape
                print(f"ℹ️  Grid shape detected: {expected_shape}. Only grids of this size will be used.")
                grids.append(grid)
            elif grid.shape == expected_shape:
                grids.append(grid)
            # If the shape does not match, the frame is simply ignored.

    # Report how many frames were discarded
    bad_frames = initial_grid_count - len(grids)
    if bad_frames > 0:
        print(f"⚠️  Warning: {bad_frames} frames were discarded due to inconsistent sizes.")
        
    return grids

# --- CONFIGURATION ---
LOG_FILE_PATH = "code/viz_data.log"

# --- ✨ Try out some cool new color maps! ---
# 'plasma', 'inferno', 'magma', 'cividis', 'hot', 'jet'
COLOR_MAP = 'plasma'

# --- MAIN CODE ---
grid_data = parse_grid_data(LOG_FILE_PATH)

if grid_data:
    print(f"✅ Processing {len(grid_data)} valid frames. Starting the rendering process...")
    print("⏳ This may take several minutes, please wait...")

    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')

    max_energy = max(grid.max() for grid in grid_data)
    grid_shape = grid_data[0].shape
    x = np.arange(grid_shape[1])
    y = np.arange(grid_shape[0])
    X, Y = np.meshgrid(x, y)

    def update(frame):
        ax.clear()
        current_grid = grid_data[frame]
        # Use vmin and vmax in plot_surface to stabilize the color bar
        ax.plot_surface(X, Y, current_grid, cmap=COLOR_MAP, edgecolor='none', vmin=0, vmax=max_energy)
        
        ax.set_title('3D Energy Diffusion', fontsize=16)
        ax.set_xlabel('X-axis')
        ax.set_ylabel('Y-axis')
        ax.set_zlabel('Energy')
        ax.set_zlim(0, max_energy)
        
        ax.text2D(0.05, 0.95, f'Time Step: {frame}', transform=ax.transAxes, fontsize=12,
                  bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.5))
        
        # You can also play with the camera angle here (elevation and azimuth)
        ax.view_init(elev=45, azim=-120)

    ani = animation.FuncAnimation(fig, update, frames=len(grid_data), interval=100)

    try:
        ani.save('energy_diffusion_3d_animated.gif', writer='imagemagick', fps=10, 
                 progress_callback=lambda i, n: print(f'    Saving frame {i+1} of {n}...'))
        print("\n✅ 3D animation saved as 'energy_diffusion_3d_animated.gif'!")
    except Exception as e:
        print(f"\n⚠️  Could not save as GIF ({e}). Attempting to save as an MP4 video instead.")
        try:
            ani.save('energy_diffusion_3d_animated.mp4', writer='ffmpeg', fps=10,
                     progress_callback=lambda i, n: print(f'    Saving frame {i+1} of {n}...'))
            print("\n✅ 3D animation saved as 'energy_diffusion_3d_animated.mp4'!")
        except Exception as e2:
             print(f"\n❌ ERROR: Could not save as MP4 either. ({e2})")
    
    plt.close(fig)

else:
    print("❌ No valid grid data found in the log file.")