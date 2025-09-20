#!/bin/bash

# --- Local Test Configuration ---
EXEC="code/stencil_parallel"
SIZE_X=100
SIZE_Y=100
ITER=10

# --- Set Environment Manually ---
# Set the number of threads for OpenMP
export OMP_NUM_THREADS=4

# Set the number of MPI processes to simulate
MPI_PROCESSES=4

echo "--- Running Local Test ---"
echo "MPI Processes: $MPI_PROCESSES"
echo "OMP Threads per process: $OMP_NUM_THREADS"
echo "--------------------------"

# Use 'mpirun' directly to run your program
# The -np flag tells mpirun how many processes to create.
mpirun -np $MPI_PROCESSES $EXEC -x $SIZE_X -y $SIZE_Y -n $ITER

echo "--- Local Test Finished ---"