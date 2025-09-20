For this project, I implemented a hybrid MPI + OpenMP solution to parallelize a 2D heat diffusion simulation. My main focus was on creating an efficient and scalable code structure. This README explains the key functions I added and the design decisions behind them.

## Key Functions and Design Choices

The most critical optimization I implemented was to **overlap computation with communication**. To achieve this, I decided to refactor the original update logic into three distinct functions: `exchange_halos`, `update_interior`, and `update_borders`.

### `exchange_halos`
To keep the communication logic clean and separate, I created the `exchange_halos` function.
* **What it does:** This function is responsible for all MPI communication. It prepares the send buffers (copying data for East/West columns) and uses non-blocking calls (`MPI_Isend` and `MPI_Irecv`) to start the data exchange with all neighbors simultaneously.
* **Why I did it:** Using non-blocking calls is essential. It allows my program to start the data transfer and immediately proceed to other tasks instead of waiting idly for the network.

### `update_interior`
* **What it does:** This function computes the new values for all the *interior* points of the local grid patch. These are all the points that are **not** on the immediate border.
* **Why I did it:** The key insight here is that the interior points do not depend on the halo data from neighboring processes. By isolating this computation, I can execute it right after starting the communication, effectively hiding the network latency behind useful work.

### `update_borders`
* **What it does:** This function calculates the new values for only the points on the outermost border of the local grid.
* **Why I did it:** These are the only points that depend on the halo data that was just received from neighboring processes. This function is only called *after* the non-blocking communication initiated by `exchange_halos` has completed.

### The Overall Strategy

By splitting the update logic this way, I structured my main loop to be highly efficient:

1.  **Call `exchange_halos`**: Start sending and receiving border data. The program does not wait.
2.  **Call `update_interior`**: While the network is busy, the CPU computes the bulk of the grid points.
3.  **Wait for communication**: A call to `MPI_Waitall` ensures the halo data has arrived.
4.  **Call `update_borders`**: With the updated halo data, the remaining border points are computed.

This design is central to my parallel solution, as it minimizes idle time and is key to achieving good scalability.
