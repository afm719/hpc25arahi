#define MAX_SIZE 10000
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <math.h>

#include <omp.h>
#include <mpi.h>

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

#define SEND 0
#define RECV 1

#define OLD 0
#define NEW 1

#define _x_ 0
#define _y_ 1

#define ARTIFICIAL_WORKLOAD 500
#define TILE_DIM 32

// Add typedef for 2D coordinate/size arrays
typedef unsigned int vec2_t[2];


typedef unsigned int uint; // just to make the code more readable

typedef double *restrict buffers_t[4]; // buffers for NORTH, SOUTH, EAST, WEST


typedef struct
{
    double *restrict data;
    vec2_t size;
} plane_t;

extern int inject_energy(const int, const int, const vec2_t *, const double, plane_t *, const vec2_t);
extern int update_plane(const int,
                        const vec2_t,
                        const plane_t *,
                        plane_t *);

/*
 *
 * This function prepares send buffers (copying for non-contiguous East/West data,
 * pointing for contiguous North/South data) and posts non-blocking MPI sends
 * and receives for all four directions. This allows for overlapping communication
 * with computation.
 */
extern void exchange_halos(plane_t* plane, uint neighbours[4], MPI_Comm comm, MPI_Request* reqs, int* req_count, MPI_Datatype col_type);

extern int get_total_energy(plane_t *,
                            double *);
extern int update_interior(const vec2_t   N, const plane_t *oldplane, plane_t *newplane); // NEW FUNCTION update_interior: updates the interior points of the plane (excluding ghost cells)
extern int update_borders(const int periodic, const vec2_t N, const plane_t *oldplane, plane_t *newplane); // NEW FUNCTION update_borders: updates the border points of the plane (including ghost cells if periodic)


int initialize(MPI_Comm *,
               int,
               int,
               int,
               char **,
               vec2_t *,
               vec2_t *,
               int *,
               int *,
               int *,
               int *,
               int *,
               int *,
               vec2_t **,
               double *,
               plane_t *,
               buffers_t *);

int memory_release(plane_t *planes, buffers_t *buffers);

int output_energy_stat(int,
                       plane_t *,
                       double,
                       int,
                       MPI_Comm);

void output_full_grid(int Me,
                      int Ntasks,
                      vec2_t mysize,
                      plane_t* plane,
                      MPI_Comm comm);



inline int inject_energy(const int periodic,
                         const int Nsources,
                         const vec2_t *Sources,
                         const double energy,
                         plane_t *plane,
                         const vec2_t N)
{
    const uint register sizex = plane->size[_x_] + 2;
    double *restrict data = plane->data;

#define IDX(i, j) ((j) * sizex + (i))
    for (int s = 0; s < Nsources; s++)
    {
        int x = Sources[s][_x_];
        int y = Sources[s][_y_];

        data[IDX(x, y)] += energy;

        if (periodic)
        {
            if ((N[_x_] == 1))
            {
                // north from south
                // south from north
                data[IDX(0, y)] = data[IDX(sizex - 2, y)];
                data[IDX(sizex - 1, y)] = data[IDX(1, y)];
            }

            if ((N[_y_] == 1))
            {
                // propagate the boundaries if needed
                // check the serial version
                data[IDX(x, 0)] = data[IDX(x, sizex - 2)];
                data[IDX(x, sizex - 1)] = data[IDX(x, 1)];
            }
        }
    }
#undef IDX

    return 0;
}

inline int update_plane(const int periodic,
                        const vec2_t N, // the grid of MPI tasks
                        const plane_t *oldplane,
                        plane_t *newplane)

{
    uint register fxsize = oldplane->size[_x_] + 2;
    uint register fysize = oldplane->size[_y_] + 2;

    uint register xsize = oldplane->size[_x_];
    uint register ysize = oldplane->size[_y_];

#define IDX(i, j) ((j) * fxsize + (i))

    // HINT: you may attempt to
    //       (i)  manually unroll the loop
    //       (ii) ask the compiler to do it
    // for instance
    // #pragma GCC unroll 4
    //
    // HINT: in any case, this loop is a good candidate
    //       for openmp parallelization

    double *restrict old = oldplane->data;
    double *restrict new = newplane->data;

    #pragma omp parallel for
    for (uint j = 1; j <= ysize; j++)
        for (uint i = 1; i <= xsize; i++)
        {

            // NOTE: (i-1,j), (i+1,j), (i,j-1) and (i,j+1) always exist even
            //       if this patch is at some border without periodic conditions;
            //       in that case it is assumed that the +-1 points are outside the
            //       plate and always have a value of 0, i.e. they are an
            //       "infinite sink" of heat

            // five-points stencil formula
            //
            // HINT : check the serial version for some optimization
            //
            new[IDX(i, j)] =
                old[IDX(i, j)] / 2.0 + (old[IDX(i - 1, j)] + old[IDX(i + 1, j)] +
                                        old[IDX(i, j - 1)] + old[IDX(i, j + 1)]) /
                                           4.0 / 2.0;
        }

    if (periodic)
    {
        if (N[_x_] == 1)
        {
            // propagate the boundaries as needed
            // check the serial version
            #pragma GCC unroll 4
            for (uint j = 1; j <= ysize; j++)
            {
                // north from south
                new[IDX(0, j)] = new[IDX(fxsize - 2, j)];
                // south from north
                new[IDX(fxsize - 1, j)] = new[IDX(1, j)];
            }
        }

        if (N[_y_] == 1)
        {
            // propagate the boundaries as needed
            // check the serial version
            #pragma GCC unroll 4
            for (uint i = 1; i <= xsize; i++)
            {
                // west from east
                new[IDX(i, 0)] = new[IDX(i, fysize - 2)];
                // east from west -> This was writing out of bounds
                new[IDX(i, ysize + 1)] = new[IDX(i, 1)];
            }
        }
    }

#undef IDX
    return 0;
}

inline int get_total_energy(plane_t *plane, double *energy)
/*
 * NOTE: this routine a good candiadate for openmp
 *       parallelization
 */
{

    const int register xsize = plane->size[_x_];
    const int register ysize = plane->size[_y_];
    const int register fsize = xsize + 2;

    double *restrict data = plane->data;

#define IDX(i, j) ((j) * fsize + (i))

#if defined(LONG_ACCURACY)
    long double totenergy = 0;
#else
    double totenergy = 0;
#endif

    // HINT: you may attempt to
    //       (i)  manually unroll the loop
    //       (ii) ask the compiler to do it
    // for instance
    // #pragma GCC unroll 4

#pragma omp parallel for reduction(+ : totenergy)
    for (int j = 1; j <= ysize; j++)
        for (int i = 1; i <= xsize; i++)
            totenergy += data[IDX(i, j)];

#undef IDX

    *energy = (double)totenergy;
    return 0;
}


inline int update_interior(const vec2_t   N,         // the grid of MPI tasks
                          const plane_t *oldplane,
                                plane_t *newplane) {
    uint sizeX = oldplane->size[_x_];
    uint sizeY = oldplane->size[_y_];
    uint frame_size = sizeX + 2;
    const double * restrict old_data = oldplane->data;
    double * restrict new_data = newplane->data;

    #pragma omp parallel for collapse(2)
    for (uint y_block = 2; y_block < sizeY; y_block += TILE_DIM) {
        for (uint x_block = 2; x_block < sizeX; x_block += TILE_DIM) {
            
            for (uint y = y_block; y < y_block + TILE_DIM && y < sizeY; y++) {
                for (uint x = x_block; x < x_block + TILE_DIM && x < sizeX; x++) {
                    
                    uint index = y * frame_size + x;
                    
                    double laplacian = old_data[index - 1] + old_data[index + 1] +
                                       old_data[index - frame_size] + old_data[index + frame_size] -
                                       4.0 * old_data[index];
                    
                    double new_value = old_data[index] + 0.1 * laplacian;

                    #if defined(ARTIFICIAL_WORKLOAD) && ARTIFICIAL_WORKLOAD > 0
                    volatile double dummy = 0.0;
                    for (int k = 0; k < ARTIFICIAL_WORKLOAD; ++k) {
                        dummy += 0.0001;
                    }
                    new_value += dummy * 0.0;
                    #endif

                    new_data[index] = new_value;
                }
            }
        }
    }
    return 0;
}

inline int update_borders(const int periodic,
                          const vec2_t N,         // the grid of MPI tasks
                          const plane_t *oldplane,
                                plane_t *newplane) {
    uint sizeX = oldplane->size[_x_];
    uint sizeY = oldplane->size[_y_];
    uint frame_size = sizeX + 2;
    const double * restrict old_data = oldplane->data;
    double * restrict new_data = newplane->data;

    #pragma omp parallel for
    for (uint x_block = 1; x_block < sizeX + 1; x_block += TILE_DIM) {
        for (uint x = x_block; x < x_block + TILE_DIM && x < sizeX + 1; x++) {
            uint index_n = 1 * frame_size + x;
            double laplacian_n = old_data[index_n - 1] + old_data[index_n + 1] + old_data[index_n - frame_size] + old_data[index_n + frame_size] - 4.0 * old_data[index_n];
            double new_value_n = old_data[index_n] + 0.1 * laplacian_n;
            
            #if defined(ARTIFICIAL_WORKLOAD) && ARTIFICIAL_WORKLOAD > 0
            volatile double dummy_n = 0.0;
            for (int k = 0; k < ARTIFICIAL_WORKLOAD; ++k) { dummy_n += 0.0001; }
            new_value_n += dummy_n * 0.0;
            #endif
            new_data[index_n] = new_value_n;


            uint index_s = sizeY * frame_size + x;
            double laplacian_s = old_data[index_s - 1] + old_data[index_s + 1] + old_data[index_s - frame_size] + old_data[index_s + frame_size] - 4.0 * old_data[index_s];
            double new_value_s = old_data[index_s] + 0.1 * laplacian_s;

            #if defined(ARTIFICIAL_WORKLOAD) && ARTIFICIAL_WORKLOAD > 0
            volatile double dummy_s = 0.0;
            for (int k = 0; k < ARTIFICIAL_WORKLOAD; ++k) { dummy_s += 0.0001; }
            new_value_s += dummy_s * 0.0;
            #endif
            new_data[index_s] = new_value_s;
        }
    }

    #pragma omp parallel for
    for (uint y_block = 2; y_block < sizeY; y_block += TILE_DIM) {
        for (uint y = y_block; y < y_block + TILE_DIM && y < sizeY; y++) {
            uint index_w = y * frame_size + 1;
            double laplacian_w = old_data[index_w - 1] + old_data[index_w + 1] + old_data[index_w - frame_size] + old_data[index_w + frame_size] - 4.0 * old_data[index_w];
            double new_value_w = old_data[index_w] + 0.1 * laplacian_w;

            #if defined(ARTIFICIAL_WORKLOAD) && ARTIFICIAL_WORKLOAD > 0
            volatile double dummy_w = 0.0;
            for (int k = 0; k < ARTIFICIAL_WORKLOAD; ++k) { dummy_w += 0.0001; }
            new_value_w += dummy_w * 0.0;
            #endif
            new_data[index_w] = new_value_w;

            uint index_e = y * frame_size + sizeX;
            double laplacian_e = old_data[index_e - 1] + old_data[index_e + 1] + old_data[index_e - frame_size] + old_data[index_e + frame_size] - 4.0 * old_data[index_e];
            double new_value_e = old_data[index_e] + 0.1 * laplacian_e;
            
            #if defined(ARTIFICIAL_WORKLOAD) && ARTIFICIAL_WORKLOAD > 0
            volatile double dummy_e = 0.0;
            for (int k = 0; k < ARTIFICIAL_WORKLOAD; ++k) { dummy_e += 0.0001; }
            new_value_e += dummy_e * 0.0;
            #endif
            new_data[index_e] = new_value_e;
        }
    }

    return 0;
}   