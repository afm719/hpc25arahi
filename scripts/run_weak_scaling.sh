#!/bin/bash
#SBATCH --job-name=weak_scale
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH --time=00:20:00
#SBATCH --array=0-5 # 6 jobs for 1, 2, 4, 8, 16, 32 nodes

# --- Experiment Configuration ---
EXEC="../code/stencil_parallel"
BASE_SIZE_PER_NODE=10000 # Problem size dimension per node
ITER=200
OUT_FILE="plots/weak/weak_scaling_results.csv"

# --- Parallel Configuration ---
THREADS_PER_TASK=8
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

# Calculate problem size to keep workload per task constant
# Workload ~ SIZE_X * SIZE_Y. If N nodes, total area ~ N. So, SIZE_X ~ sqrt(N)
SIZE_X=$(echo "sqrt($NODES) * $BASE_SIZE_PER_NODE" | bc -l | awk '{printf "%d", $1}')
SIZE_Y=$BASE_SIZE_PER_NODE

# The first job creates the CSV header
if [ $SLURM_ARRAY_TASK_ID -eq 0 ]; then
    echo "Nodes,Total_Tasks,Size_X,Size_Y,Total_Time" > $OUT_FILE
fi

echo "Running Weak Scaling on $NODES nodes (Size: ${SIZE_X}x${SIZE_Y})..."

# Execute and parse the max total time
TIME=$(srun -N $NODES -n $TOTAL_TASKS --ntasks-per-node=$TASKS_PER_NODE --cpus-per-task=$THREADS_PER_TASK \
     $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER | grep "Total Time" | awk '{print $4}')

# Append the result safely to the CSV file
flock -x $OUT_FILE -c "echo $NODES,$TOTAL_TASKS,$SIZE_X,$SIZE_Y,$TIME >> $OUT_FILE"