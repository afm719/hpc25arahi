#!/bin/bash
echo "Submitting Weak Scaling jobs..."

# --- Test Configuration ---
# The problem size per node is constant
BASE_SIZE_PER_NODE=10000
ITER=200
THREADS_PER_TASK=8
TASKS_PER_NODE=12 # 12*8 = 96 cores per node, a safe value

# Create the CSV header once before submitting any jobs
# We add Size_X and Size_Y to see how the problem grows
echo "Nodes,Total_Tasks,Size_X,Size_Y,Total_Time,Comm_Time,Compute_Time,Wait_Time" > plots/weak_scaling_results.csv

# Loop to submit one job for each node count
for NODES in 1 2 4 8 16 32; do
    TOTAL_TASKS=$((NODES * TASKS_PER_NODE))
    
    # Calculate the global problem size for this run.
    # The area (X*Y) scales linearly with the number of nodes,
    # so the side length (X and Y) scales with sqrt(NODES).
    SIZE_X=$(echo "sqrt($NODES) * $BASE_SIZE_PER_NODE" | bc -l | awk '{printf "%d", $1}')
    SIZE_Y=$SIZE_X # Keeping the domain square for simplicity

    echo "Submitting job for $NODES nodes with grid size ${SIZE_X}x${SIZE_Y}..."
    # This command submits the 'worker' script with specific resources and variables for this loop iteration
    sbatch --nodes=${NODES} \
           --ntasks=${TOTAL_TASKS} \
           --ntasks-per-node=${TASKS_PER_NODE} \
           --cpus-per-task=${THREADS_PER_TASK} \
           --job-name="weak_${NODES}n" \
           --export=ALL,SIZE_X=${SIZE_X},SIZE_Y=${SIZE_Y},ITER=${ITER},THREADS_PER_TASK=${THREADS_PER_TASK},TASKS_PER_NODE=${TASKS_PER_NODE} \
           scripts/run_weak_scaling.sh
done

echo "All jobs submitted."