# Parallel Stencil Computation for the 2D Heat Equation

This repository contains a high-performance, hybrid MPI and OpenMP implementation for solving the 2D heat equation using a 5-point stencil method. The project was developed as part of the High-Performance Computing course at the University of Trieste.

The primary goal was to parallelize a serial C template to create a robust, scalable application capable of running efficiently on a modern supercomputing architecture. The implementation includes advanced optimization techniques such as computation/communication overlap and cache tiling.


![energy_diffusion_3d_animated](https://github.com/user-attachments/assets/7e9517c2-6b88-40b9-892f-12ea0e83295a)


## Features 

  - **Hybrid Parallelization:** Combines MPI for distributed-memory parallelism across nodes and OpenMP for shared-memory parallelism within a single node.
  - **Domain Decomposition:** Decomposes the 2D grid into a Cartesian topology of MPI processes.
  - **Computation/Communication Overlap:** Implements a sophisticated main loop that overlaps the calculation of the grid's interior with the non-blocking communication of boundary data (halos).
  - **Cache Tiling Optimization:** The OpenMP loops are optimized with a tiling (blocking) strategy to maximize cache reuse and improve single-node performance.
  - **Efficient Communication:** Uses `MPI_Type_vector` to efficiently handle the communication of non-contiguous data in grid columns.
  - **Performance Instrumentation:** Includes timers (`MPI_Wtime`) to measure and analyze the time spent in computation vs. communication, allowing for detailed performance analysis.
  - **Scalability Analysis:** The code has been benchmarked with strong, weak, and OpenMP scaling studies.

-----

## Environment

The code was developed and benchmarked on the **Leonardo DCGP Cluster**.

  * **Supercomputer:** Leonardo DCGP
  * **Node Architecture:** 2x Sockets, 56 cores/socket (112 cores total)
  * **Job Submission:** SLURM batch scripts

-----


### Domain Decomposition & Halo Exchange

The core parallelization strategy is domain decomposition. The global 2D grid is divided among MPI processes. Each process is only responsible for computing its local sub-grid.

To calculate the values at the boundaries of a sub-grid, each process needs data from its immediate neighbors. This is achieved through **Halo Exchange**:

1.  Each process allocates extra memory around its local grid—these are the "halos" or "ghost cells".
2.  Before computation, each process exchanges its boundary data with its neighbors.
3.  The received data is stored in the corresponding halo cells.
4.  This allows the stencil computation at the boundaries to proceed as if it re on a larger, continuous grid.

### Computation/Communication Overlap

To hide the latency of network communication, the main loop is structured to overlap tasks. This is the single most important performance optimization in the code.

1.  **`exchange_halos()`:** Non-blocking communication (`MPI_Isend`, `MPI_Irecv`) is initiated for all halos. This function returns immediately, allowing the program to continue.
2.  **`update_interior()`:** While the halo data is in transit over the network, the CPU computes the new values for the *interior* points of the grid. These points do not depend on the halo data, so this work is "safe" to perform.
3.  **`MPI_Waitall()`:** The program reaches a synchronization point where it waits for all the non-blocking communications to complete.
4.  **`update_borders()`:** Once the halo data has been received, the program computes the new values for the boundary points.

This strategy ensures the CPU is kept busy with useful computation instead of idly waiting for data to arrive.

### OpenMP Tiling for Cache Efficiency

Within each MPI process, the loops in `update_interior` and `update_borders` are parallelized with OpenMP. To maximize performance, a **tiling** (or cache-blocking) technique is used.

  - **Problem:** Iterating over a large grid row-by-row causes poor data locality, leading to frequent cache misses.
  - **Solution:** The local grid is partitioned into small, cache-friendly square tiles. OpenMP threads work on one tile at a time. This ensures that data loaded into the fast L1/L2 cache is reused multiple times before being evicted, significantly reducing trips to the much slor main memory.

-----

## Quick Command Guide for the Project on Leonardo

This guide summarizes the entire workflow, from accessing the cluster to running all performance and visualization tests.

### 1\. Accessing the Cluster

Connect to the Leonardo login node using `ssh` with your user credentials.

```bash
ssh afernan3@login.leonardo.cineca.it
```

-----

### 2\. Compiling the Code

The parallel code is compiled by submitting a job to the system. This ensures a clean and correct compilation environment.

```bash
# Submit the compilation script
sbatch scripts/compile.sh
```

To check the output and verify that the compilation was successful, view the Slurm output file:

```bash
# Replace XXXXXX with the job ID
cat slurm-XXXXXX.out
```

-----

### 3\. Running the Performance Tests

Each test is launched using its specific script.

#### OpenMP Scaling Test

This script submits a job array to test performance with different thread counts (1, 2, 4, 8, 16, 32, 56, 84, 112) on a single node.

```bash
sbatch scripts/run_openmp_scaling.sh
```

  * **Result:** Data is saved to `plots/openmp_scaling/openmp_scaling_metrics.csv`.

#### Strong Scaling Test

This script launches jobs on 1, 2, 4, 8, and 16 nodes with a fixed, large problem size. The most successful configuration I found was:

  * **Problem Size:** `15000x15000`
  * **MPI Tasks per Node:** 8
  * **OpenMP Threads per Task:** 14

<!-- end list -->

```bash
# Grant execution permissions to the launcher script
chmod +x launch_strong_scaling.sh

# Run the launcher
sbatch ./launch_strong_scaling.sh
```

  * **Result:** Data is appended to `plots/strong_scaling_results.csv`.

#### Weak Scaling Test

This script launches jobs on multiple nodes, increasing the problem size proportionally.

  * **Base Size per Node:** `10000x10000`

<!-- end list -->

```bash
# Grant execution permissions to the launcher script
chmod +x launch_weak_scaling.sh

# Run the launcher
sbatch ./launch_weak_scaling.sh
```

  * **Result:** Data is appended to `plots/weak_scaling_results.csv`.

-----



### 3\. Job Monitoring & Management

Useful commands for managing your jobs on the cluster.

  * **View the status of your jobs in the queue:**
    ```bash
    squeue -u afernan3
    ```
  * **Cancel a specific job:**
    ```bash
    scancel [JOB_ID]
    ```
  * **Cancel ALL of your jobs:**
    ```bash
    scancel -u afernan3
    ```

The application was benchmarked to evaluate its scalability.

  * **Strong Scaling:** The code showed good speedup, but efficiency dropped as the number of nodes increased. This is expected and explained by Amdahl's Law, as the fixed communication overhead becomes more significant relative to the shrinking computational work per process.
  * **Weak Scaling:** The code demonstrated excellent weak scalability, with efficiency remaining high and nearly flat (around 80-90%). This proves the implementation is highly scalable and can solve larger problems by adding more resources.
  * **OpenMP Scaling:** On a single node, the code showed good speedup with an increasing number of threads, validating the effectiveness of the shared-memory parallelism and the tiling optimization.

-----

