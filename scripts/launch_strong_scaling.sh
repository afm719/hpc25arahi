#!/bin/bash
echo "Submitting Strong Scaling jobs..."

# --- Test Configuration ---
SIZE_X=10000
SIZE_Y=10000
ITER=200
THREADS_PER_TASK=8
TASKS_PER_NODE=12 # 12*8 = 96 cores per node, a safe value

# Create the CSV header once before submitting any jobs
echo "Nodes,Total_Tasks,Total_Time,Comm_Time,Compute_Time,Wait_Time" > plots/strong_scaling_results.csv

# Loop to submit one job for each node count
for NODES in 1 2 4 8 16; do
    TOTAL_TASKS=$((NODES * TASKS_PER_NODE))
    
    echo "Submitting job for $NODES nodes..."
    # This command submits the 'worker' script with specific resources for this loop iteration
    sbatch --nodes=${NODES} \
           --ntasks=${TOTAL_TASKS} \
           --ntasks-per-node=${TASKS_PER_NODE} \
           --cpus-per-task=${THREADS_PER_TASK} \
           --job-name="strong_${NODES}n" \
           --export=ALL,SIZE_X=${SIZE_X},SIZE_Y=${SIZE_Y},ITER=${ITER},THREADS_PER_TASK=${THREADS_PER_TASK} \
           scripts/run_strong_scaling.sh
done

echo "All jobs submitted."
