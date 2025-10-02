#!/bin/bash
#SBATCH --job-name=weak_scale
#SBATCH --partition=dcgp_usr_prod
#SBATCH -A uTS25_Tornator_0
#SBATCH --time=00:45:00


# --- Base Directory ---
BASE_DIR=$SLURM_SUBMIT_DIR

# --- Test Configuration (values are passed from the launcher via --export) ---
EXEC="$BASE_DIR/code/stencil_parallel"
OUT_FILE="$BASE_DIR/plots/weak_scaling_results.csv"

# --- Execution Environment ---
module purge
module load openmpi/4.1.6--gcc--12.2.0
export OMP_NUM_THREADS=$THREADS_PER_TASK
export OMP_PLACES=threads
export OMP_PROC_BIND=close

echo "Running Weak Scaling on $SLURM_NNODES nodes with $SLURM_NTASKS tasks (Size: ${SIZE_X}x${SIZE_Y})..."

# --- Execution and Result Capture ---
PROGRAM_OUTPUT=$(srun $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER)
METRICS_LINE=$(echo "$PROGRAM_OUTPUT" | grep "CSV_DATA")
TOTAL_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $2}')
COMM_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $3}')
COMP_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $4}')
WAIT_TIME=$(echo "$METRICS_LINE" | awk -F',' '{print $5}')

# --- Safe Result Writing ---
# We add Size_X and Size_Y to the CSV output
if [ -z "$TOTAL_TIME" ]; then
    flock -x $OUT_FILE -c "echo $SLURM_NNODES,$SLURM_NTASKS,$SIZE_X,$SIZE_Y,ERROR,ERROR,ERROR,ERROR >> $OUT_FILE"
else
    flock -x $OUT_FILE -c "echo $SLURM_NNODES,$SLURM_NTASKS,$SIZE_X,$SIZE_Y,$TOTAL_TIME,$COMM_TIME,$COMP_TIME,$WAIT_TIME >> $OUT_FILE"
fi