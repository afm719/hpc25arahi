import numpy as np
import matplotlib.pyplot as plt
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
    for block in time_step_blocks:
        current_grid_lines = []
        lines_in_block = block.strip().split('\n')
        for line in lines_in_block:
            line = line.strip()
            # Ignore non-data lines
            if not line or line.startswith("[DEBUG]") or not line[0].isdigit():
                continue
            try:
                row_data = [float(x) for x in line.split()]
                if row_data:
                    current_grid_lines.append(row_data)
            except ValueError:
                continue
        
        if current_grid_lines and all(len(row) == len(current_grid_lines[0]) for row in current_grid_lines):
            grid = np.array(current_grid_lines)
            if expected_shape is None:
                expected_shape = grid.shape
                print(f"ℹ️  Grid shape detected: {expected_shape}. Only grids of this size will be used.")
            
            if grid.shape == expected_shape:
                grids.append(grid)
            
    return grids

# --- CONFIGURATION ---
LOG_FILE_PATH = "code/viz_data.log"

# --- ✨ Choose the frames you want to view here! ---
# This is now a list. Add any frame numbers you want to see.
FRAMES_TO_VISUALIZE = [0, 10, 20, 30,40,49] 

# --- Choose the heatmap style ---
COLOR_MAP = 'plasma' 
INTERPOLATION_METHOD = 'bilinear'

# --- MAIN CODE ---
grid_data = parse_grid_data(LOG_FILE_PATH)

if grid_data:
    print(f"✅ Processed {len(grid_data)} valid frames.")
    
    # Loop through the list of frames you want to generate
    for frame_number in FRAMES_TO_VISUALIZE:
        # Check if the requested frame exists
        if frame_number < len(grid_data):
            print(f"🖼️  Generating 2D heatmap for frame number {frame_number}...")

            # Select the specific frame
            specific_frame_grid = grid_data[frame_number]
            
            # Create the figure and plot
            plt.figure(figsize=(12, 12))
            plt.imshow(specific_frame_grid, cmap=COLOR_MAP, interpolation=INTERPOLATION_METHOD)
            
            # Add details to the plot
            plt.colorbar(label='Energy')
            plt.title(f'2D Energy Heatmap (Time Step: {frame_number})', fontsize=16)
            plt.xlabel('X-axis')
            plt.ylabel('Y-axis')
            
            # Save the image to a unique file
            output_filename = f'heatmap_2d_frame_{frame_number}.png'
            plt.savefig(output_filename)
            plt.close()
            
            print(f"✅ Plot saved as '{output_filename}'!")

        else:
            print(f"❌ Warning: Frame {frame_number} does not exist and will be skipped. (Max frame: {len(grid_data) - 1})")
else:
    print("❌ No valid grid data found in the log file.")