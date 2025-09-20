#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH --time=00:30:00
#SBATCH --job-name=openmp_scale
#SBATCH --array=0-8 # 9 jobs for 9 thread counts

# --- Experiment Configuration ---
EXEC="./stencil_parallel"
SIZE_X=20000
SIZE_Y=20000
ITER=100
OUT_FILE="plots/openmp_scaling/openmp_scaling_results.csv"

# --- Modules and Environment ---
module load gcc/12.2.0
module load openmpi/4.1.6--gcc--12.2.0

# --- Set Thread Affinity (good for performance) ---
# OMP_PLACES=threads: Each thread gets its own set of hardware resources.
# OMP_PROC_BIND=close: Binds threads closely to the master thread's resources.
export OMP_PLACES=threads
export OMP_PROC_BIND=close

# Array of thread counts to test
declare -a thread_counts=(1 2 4 8 16 32 56 84 112)
# Get the thread count for this specific job array task
THREADS=${thread_counts[$SLURM_ARRAY_TASK_ID]}
export OMP_NUM_THREADS=$THREADS

# The first job in the array creates the CSV header
if [ $SLURM_ARRAY_TASK_ID -eq 0 ]; then
    echo "Threads,Total_Time" > $OUT_FILE
fi

echo "Running OpenMP test with $THREADS threads..."

# Execute and parse the max total time from the output
TIME=$(srun --cpus-per-task=$THREADS $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER | grep "Total Time" | awk '{print $4}')

# Append the result safely to the CSV file
flock -x $OUT_FILE -c "echo $THREADS,$TIME >> $OUT_FILE"

# The last job signals completion
if [ $SLURM_ARRAY_TASK_ID -eq 8 ]; then
    echo "OpenMP scaling study finished. Results are in $OUT_FILE"
fi