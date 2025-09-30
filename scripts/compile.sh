#!/bin/bash
#SBATCH --job-name=compilation
#SBATCH --nodes=1
#SBATCH --partition=dcgp_usr_prod
#SBATCH --time=00:10:00
#SBATCH -A uTS25_Tornator_0


# Base directory (where sbatch was launched from)
BASE_DIR=$SLURM_SUBMIT_DIR

# Clean the module environment to avoid conflicts
module purge
module load openmpi/4.1.6--gcc--12.2.0

echo "== Compiling code in $BASE_DIR/code/ =="

make -C "$BASE_DIR/code/"
echo "== Compilation finished."
