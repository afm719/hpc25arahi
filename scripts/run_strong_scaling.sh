#!/bin/bash
#SBATCH --job-name=strong_scale
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH --time=00:20:00
#SBATCH --array=0-5 # 6 jobs for 1, 2, 4, 8, 16, 32 nodes

# --- Experiment Configuration ---
EXEC="../code/stencil_parallel"
SIZE_X=40000  # Fixed problem size
SIZE_Y=40000
ITER=200
OUT_FILE="plots/strong/strong_scaling_results.csv"

# --- Parallel Configuration ---
# Set the optimal number of threads you found in the OpenMP test
THREADS_PER_TASK=8
# Set how many MPI tasks you want to run on each node
TASKS_PER_NODE=14

# --- Modules and Environment ---
module load gcc/12.2.0
module load openmpi/4.1.6--gcc--12.2.0
export OMP_NUM_THREADS=$THREADS_PER_TASK
export OMP_PLACES=threads
export OMP_PROC_BIND=close

# --- SLURM Job Array Logic ---
declare -a nodes_array=(1 2 4 8 16 32)
NODES=${nodes_array[$SLURM_ARRAY_TASK_ID]}
TOTAL_TASKS=$(( $TASKS_PER_NODE * $NODES ))

# The first job creates the CSV header
if [ $SLURM_ARRAY_TASK_ID -eq 0 ]; then
    echo "Nodes,Total_Tasks,Total_Time" > $OUT_FILE
fi

echo "Running Strong Scaling on $NODES nodes with $TOTAL_TASKS tasks..."

# Execute and parse the max total time
TIME=$(srun -N $NODES -n $TOTAL_TASKS --ntasks-per-node=$TASKS_PER_NODE --cpus-per-task=$THREADS_PER_TASK \
     $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER | grep "Total Time" | awk '{print $4}')

# Append the result safely to the CSV file
flock -x $OUT_FILE -c "echo $NODES,$TOTAL_TASKS,$TIME >> $OUT_FILE"