#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH --cpus-per-task=112
#SBATCH --time=00:30:00
#SBATCH --job-name=openmp_scale
#SBATCH --array=0-8 # 9 jobs for 9 thread counts

# --- Base Directory ---
BASE_DIR=$SLURM_SUBMIT_DIR

# --- Test Configuration ---
EXEC="$BASE_DIR/code/stencil_parallel"
SIZE_X=2000
SIZE_Y=2000
ITER=100
OUT_FILE="$BASE_DIR/plots/openmp_scaling/openmp_scaling_metrics.csv"

# --- Execution Environment ---
module purge
module load openmpi/4.1.6--gcc--12.2.0
export OMP_PLACES=threads
export OMP_PROC_BIND=close

# --- Compilation and CSV Header (only by the first job) ---
if [ $SLURM_ARRAY_TASK_ID -eq 0 ]; then
    # New header with all metrics
    echo "Threads,Total_Time,Comm_Time,Compute_Time,Wait_Time" > $OUT_FILE
fi

# --- Job Array Logic ---
declare -a thread_counts=(1 2 4 8 16 32 56 84 112)
THREADS=${thread_counts[$SLURM_ARRAY_TASK_ID]}
export OMP_NUM_THREADS=$THREADS

echo "Running test with $THREADS threads..."

# --- Execution and Result Capture ---
PROGRAM_OUTPUT=$(srun --cpus-per-task=$THREADS $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER)

# --- Advanced Parsing for All Metrics ---
# Capture the specific CSV_DATA line your C program now prints
METRICS_LINE=$(echo "$PROGRAM_OUTPUT" | grep "CSV_DATA")

# Extract each metric using awk with a comma as a separator
TOTAL_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $2}')
COMM_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $3}')
COMP_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $4}')
WAIT_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $5}')


# --- Safe Result Writing ---
if [ -z "$TOTAL_TIME" ]; then
    flock -x $OUT_FILE -c "echo $THREADS,ERROR,ERROR,ERROR,ERROR >> $OUT_FILE"
else
    flock -x $OUT_FILE -c "echo $THREADS,$TOTAL_TIME,$COMM_TIME,$COMP_TIME,$WAIT_TIME >> $OUT_FILE"
fi