/*

/*
 *
 *  mysizex   :   local x-extendion of your patch
 *  mysizey   :   local y-extension of your patch
 *
 */


#include "stencil_template_parallel.h"



// ------------------------------------------------------------------
// ------------------------------------------------------------------

int main(int argc, char **argv)
{
  MPI_Comm myCOMM_WORLD;
  int  Rank, Ntasks;
  uint neighbours[4];

  int  Niterations;
  int  periodic;
  vec2_t S, N;
  
  int      Nsources;
  int      Nsources_local;
  vec2_t  *Sources_local;
  double   energy_per_source;
  int verbose = 0;

  plane_t   planes[2];  
  buffers_t buffers[2];
  
  int output_energy_stat_perstep;
  
  /* initialize MPI envrionment */
  {
    int level_obtained;
    
    // NOTE: change MPI_FUNNELED if appropriate
    //
    MPI_Init_thread( &argc, &argv, MPI_THREAD_FUNNELED, &level_obtained );
    if ( level_obtained < MPI_THREAD_FUNNELED ) {
      printf("MPI_thread level obtained is %d instead of %d\n",
	     level_obtained, MPI_THREAD_FUNNELED );
      MPI_Finalize();
      exit(1); }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Ntasks);
    MPI_Comm_dup (MPI_COMM_WORLD, &myCOMM_WORLD);
  }
  
  
  /* argument checking and setting */
  int ret = initialize ( &myCOMM_WORLD, Rank, Ntasks, argc, argv, &S, &N, &periodic, &output_energy_stat_perstep,
			 neighbours, &Niterations,
			 &Nsources, &Nsources_local, &Sources_local, &energy_per_source,
			 &planes[0], &buffers[0] );

  if ( ret )
    {
      printf("task %d is opting out with termination code %d\n",
	     Rank, ret );
      
      MPI_Finalize();
      return 0;
    }
  
  
  int current = OLD;
  double t1 = MPI_Wtime();   /* take wall-clock time */
  uint sizeX = planes[current].size[_x_];
  uint sizeY = planes[current].size[_y_];
  uint frame_size = (sizeX+2);

  double compute_time = 0.0;
  double comm_time = 0.0;
  double total_time = 0.0;
  double wait_time = 0.0;
  total_time = -t1;
  
  for (int iter = 0; iter < Niterations; ++iter)
    
    {
      
      MPI_Request reqs[8];
      int req_count = 0;
      /* new energy from sources */
      compute_time -= MPI_Wtime();
      inject_energy( periodic, Nsources_local, Sources_local, energy_per_source, &planes[current], N );
      compute_time += MPI_Wtime();

      /* -------------------------------------- */

      // [A] fill the buffers, and/or make the buffers' pointers pointing to the correct position

      if (neighbours[WEST] != MPI_PROC_NULL) {
          #pragma GCC unroll 4
          for (uint j = 0; j < sizeY; j++) {
              buffers[SEND][WEST][j] = planes[current].data[(j+1)*frame_size + 1];
          }
      }

      
      if (neighbours[EAST] != MPI_PROC_NULL) {
          #pragma GCC unroll 4
          for (uint j = 0; j < sizeY; j++) {
              buffers[SEND][EAST][j] = planes[current].data[(j+1)*frame_size + sizeX]; // EAST buffer filled  
          }
      } // EAST buffer filled

      //Now is necesary the grid point to be contiguous in memory
      
      if (neighbours[NORTH] != MPI_PROC_NULL) {
        buffers[SEND][NORTH] = &(planes[current].data[frame_size + 1]);
        buffers[RECV][NORTH] = &(planes[current].data[1]);
      }
      if (neighbours[SOUTH] != MPI_PROC_NULL) {
        buffers[SEND][SOUTH] = &(planes[current].data[sizeY*frame_size + 1]);
        buffers[RECV][SOUTH] = &(planes[current].data[(sizeY+1)*frame_size + 1]);
      }
      // [B] perfoem the halo communications
      //     (1) use Send / Recv
      //     (2) use Isend / Irecv
      //         --> can you overlap communication and compution in this way?
      
      comm_time -= MPI_Wtime();
      #pragma GCC unroll 4
      for (int d = 0; d < 4; d++) {
          if (neighbours[d] != MPI_PROC_NULL) {
              MPI_Irecv(buffers[RECV][d], (d < 2 ? sizeX : sizeY), MPI_DOUBLE, neighbours[d], 123, myCOMM_WORLD, &reqs[req_count++]);
              MPI_Isend(buffers[SEND][d], (d < 2 ? sizeX : sizeY), MPI_DOUBLE, neighbours[d], 123, myCOMM_WORLD, &reqs[req_count++]);
          }
      }
      comm_time += MPI_Wtime();
      compute_time -= MPI_Wtime();

      // [C] copy the haloes data

      /* --------------------------------------  */
      /* update grid points */
      
      update_interior( N, &planes[current], &planes[!current] );
      compute_time += MPI_Wtime();


      wait_time -= MPI_Wtime();
      if(req_count > 0)
          MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
      wait_time += MPI_Wtime(); 

      if(neighbours[WEST] != MPI_PROC_NULL) {
          #pragma GCC unroll 4
          for (uint j = 0; j < sizeY; j++) {
              planes[current].data[(j+1)*frame_size] = buffers[RECV][WEST][j]; // WEST halo filled
          }
      }
      if(neighbours[EAST] != MPI_PROC_NULL) {
          #pragma GCC unroll 4
          for (uint j = 0; j < sizeY; j++) {
              planes[current].data[(j+1)*frame_size + sizeX + 1] = buffers[RECV][EAST][j]; // EAST halo filled
          }
      }

      /* --------------------------------------  */
      /* update border points */
      compute_time -= MPI_Wtime();
      update_borders( periodic, N, &planes[current], &planes[!current] );
      compute_time += MPI_Wtime();


      /* output if needed */
      if ( output_energy_stat_perstep )
	        output_energy_stat ( iter, &planes[!current], (iter+1) * Nsources*energy_per_source, Rank, myCOMM_WORLD );

      // Debugging output
      if (verbose) {
        output_full_grid(Rank, Ntasks, planes[!current].size, &planes[!current], myCOMM_WORLD);
        if (Rank == 0) {
            printf("Iteration %d:\n", iter);
            for (uint j = 0; j < sizeY + 2; j++) {
                for (uint i = 0; i < sizeX + 2; i++) {
                    printf("%6.2f ", planes[!current].data[j * frame_size + i]);
                }
                printf("\n");
            }
            printf("\n");

            FILE *f = fopen("code/time/timings.csv", "a"); // "a" for append
            if (f) {
                fseek(f, 0, SEEK_END);
                if (ftell(f) == 0) { // If the file is empty, write the header
                    fprintf(f, "Processes,TotalTime,ComputeTime\n");
                }
                fprintf(f, "%d,%f,%f\n", Ntasks, total_time, compute_time);
                fclose(f);
}
        }

      }
  
	
      /* swap plane indexes for the new iteration */
      current = !current;
      
    }
  
  total_time += MPI_Wtime();

  output_energy_stat ( -1, &planes[!current], Niterations * Nsources*energy_per_source, Rank, myCOMM_WORLD );
  
  memory_release( planes, buffers );

  comm_time += wait_time;

  double max_total_time, max_comm_time, max_compute_time, max_wait_time;
  MPI_Reduce( &total_time, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, 0, myCOMM_WORLD );
  MPI_Reduce( &comm_time, &max_comm_time, 1, MPI_DOUBLE, MPI_MAX, 0, myCOMM_WORLD );
  MPI_Reduce( &compute_time, &max_compute_time, 1, MPI_DOUBLE, MPI_MAX, 0, myCOMM_WORLD );
  MPI_Reduce( &wait_time, &max_wait_time, 1, MPI_DOUBLE, MPI_MAX, 0, myCOMM_WORLD );
  if ( Rank == 0 )
    printf( "max total time is %g sec, "
      "max comm time is %g sec, "
      "max compute time is %g sec, "
      "max wait time is %g sec\n",
      max_total_time,
      max_comm_time,
      max_compute_time,
      max_wait_time
      );
  
  
  
  MPI_Finalize();
  return 0;
}


/* ==========================================================================
   =                                                                        =
   =   routines called within the integration loop                          =
   ========================================================================== */





/* ==========================================================================
   =                                                                        =
   =   initialization                                                       =
   ========================================================================== */


uint simple_factorization( uint, int *, uint ** );

int initialize_sources( int       ,
			int       ,
			MPI_Comm  *,
			uint      [2],
			int       ,
			int      *,
			vec2_t  ** );


int memory_allocate ( const int       *neighbours  ,
		      const vec2_t     N           ,
		            buffers_t *buffers_ptr ,
		            plane_t   *planes_ptr
		      )

{
    /*
      here you allocate the memory buffers that you need to
      (i)  hold the results of your computation
      (ii) communicate with your neighbours

      The memory layout that I propose to you is as follows:

      (i) --- calculations
      you need 2 memory regions: the "OLD" one that contains the
      results for the step (i-1)th, and the "NEW" one that will contain
      the updated results from the step ith.

      Then, the "NEW" will be treated as "OLD" and viceversa.

      These two memory regions are indexed by *plate_ptr:

      planew_ptr[0] ==> the "OLD" region
      plamew_ptr[1] ==> the "NEW" region


      (ii) --- communications

      you may need two buffers (one for sending and one for receiving)
      for each one of your neighnours, that are at most 4:
      north, south, east amd west.      

      To them you need to communicate at most mysizex or mysizey
      daouble data.

      These buffers are indexed by the buffer_ptr pointer so
      that

      (*buffers_ptr)[SEND][ {NORTH,...,WEST} ] = .. some memory regions
      (*buffers_ptr)[RECV][ {NORTH,...,WEST} ] = .. some memory regions
      
      --->> Of course you can change this layout as you prefer
      
     */

  printf("[DEBUG] Dentro de memory_allocate...\n");
  printf("        'planes_ptr' RECIBIDO con la dirección: %p\n", (void*)planes_ptr);
  printf("        'buffers_ptr' RECIBIDO con la dirección: %p\n", (void*)buffers_ptr);
  fflush(stdout);
  if (planes_ptr == NULL || buffers_ptr == NULL) {
        printf("Error: An invalid NULL pointer was passed to memory_allocate.\n");
        return 1;
    }
    

  // ··················································
  // allocate memory for data
  // we allocate the space needed for the plane plus a contour frame
  // that will contains data form neighbouring MPI tasks
  unsigned int frame_size = (planes_ptr[OLD].size[_x_]+2) * (planes_ptr[OLD].size[_y_]+2);

  planes_ptr[OLD].data = (double*)malloc( frame_size * sizeof(double) );
  if ( planes_ptr[OLD].data == NULL ) {
    // manage the malloc fail
    printf("task could not allocate memory for the data\n");
    return 2;
  }
  memset ( planes_ptr[OLD].data, 0, frame_size * sizeof(double) );

  planes_ptr[NEW].data = (double*)malloc( frame_size * sizeof(double) );
  if ( planes_ptr[NEW].data == NULL ) {
    // manage the malloc fail
    free ( planes_ptr[OLD].data );
    printf("task could not allocate memory for the data\n");
    return 3;
  }
  memset ( planes_ptr[NEW].data, 0, frame_size * sizeof(double) );


  // ··················································
  // buffers for north and south communication 
  // are not really needed
  //
  // in fact, they are already contiguous, just the
  // first and last line of every rank's plane
  //
  // you may just make some pointers pointing to the
  // correct positions
  //

  // or, if you preer, just go on and allocate buffers
  // also for north and south communications

  // ··················································
  // allocate buffers
  //

  buffers_ptr[SEND][NORTH] = NULL; // &(planes_ptr[OLD].data[1*(planes_ptr[OLD].size[_x_]+2) + 1]);
  buffers_ptr[SEND][SOUTH] = NULL; // &(planes_ptr[OLD].data[(planes_ptr[OLD].size[_y_])*(planes_ptr[OLD].size[_x_]+2) + 1]); 
  buffers_ptr[RECV][NORTH] = NULL; // &(planes_ptr[OLD].data[0*(planes_ptr[OLD].size[_x_]+2) + 1]);
  buffers_ptr[RECV][SOUTH] = NULL; // &(planes_ptr[OLD].data[(planes_ptr[OLD].size[_y_]+1)*(planes_ptr[OLD].size[_x_]+2) + 1]);

  if (neighbours[EAST] != MPI_PROC_NULL) {
    buffers_ptr[SEND][EAST] = (double*)calloc(planes_ptr[OLD].size[_y_], sizeof(double));
    buffers_ptr[RECV][EAST] = (double*)calloc(planes_ptr[OLD].size[_y_], sizeof(double));
  }
  if (neighbours[WEST] != MPI_PROC_NULL) {
    buffers_ptr[SEND][WEST] = (double*)calloc(planes_ptr[OLD].size[_y_], sizeof(double));
    buffers_ptr[RECV][WEST] = (double*)calloc(planes_ptr[OLD].size[_y_], sizeof(double)); 
  }
  // ··················································

  
  return 0;
}

          

int initialize ( MPI_Comm *Comm,
		 int      Me,                  // the rank of the calling process
		 int      Ntasks,              // the total number of MPI ranks
		 int      argc,                // the argc from command line
		 char   **argv,                // the argv from command line
		 vec2_t  *S,                   // the size of the plane
		 vec2_t  *N,                   // two-uint array defining the MPI tasks' grid
		 int     *periodic,            // periodic-boundary tag
		 int     *output_energy_stat,
		 int     *neighbours,          // four-int array that gives back the neighbours of the calling task
		 int     *Niterations,         // how many iterations
		 int     *Nsources,            // how many heat sources
		 int     *Nsources_local,
		 vec2_t **Sources_local,
		 double  *energy_per_source,   // how much heat per source
		 plane_t *planes,
		 buffers_t *buffers
		 )
{
  int halt = 0;
  int ret;
  int verbose = 0;
  
  // ··································································
  // set deffault values

  (*S)[_x_]         = 10000;
  (*S)[_y_]         = 10000;
  *periodic         = 0;
  *Nsources         = 4;
  *Nsources_local   = 0;
  *Sources_local    = NULL;
  *Niterations      = 1000;
  *energy_per_source = 1.0;
  *output_energy_stat = 0;
  verbose         = 0;


  if ( planes == NULL ) {
    // manage the situation
    printf("an invalid pointer has been passed\n");
    return 1;
  }

  planes[OLD].size[_x_] = 0; // This is planes_ptr[0]
  planes[OLD].size[_y_] = 0; // This is planes_ptr[0]
  planes[NEW].size[_x_] = 0; // This is planes_ptr[1]
  planes[NEW].size[_y_] = 0; // This is planes_ptr[1]
  
  for ( int i = 0; i < 4; i++ )
    neighbours[i] = MPI_PROC_NULL;

  for ( int b = 0; b < 2; b++ )
    for ( int d = 0; d < 4; d++ )
      buffers[b][d] = NULL;
  
  // ··································································
  // process the commadn line
  // 
  while ( 1 )
  {
    int opt;
    while((opt = getopt(argc, argv, ":hx:y:e:E:n:o:p:v:")) != -1)
      {
	switch( opt )
	  {
	  case 'x': (*S)[_x_] = (uint)atoi(optarg);
	    break;

	  case 'y': (*S)[_y_] = (uint)atoi(optarg);
	    break;

	  case 'e': *Nsources = atoi(optarg);
	    break;

	  case 'E': *energy_per_source = atof(optarg);
	    break;

	  case 'n': *Niterations = atoi(optarg);
	    break;

	  case 'o': *output_energy_stat = (atoi(optarg) > 0);
	    break;

	  case 'p': *periodic = (atoi(optarg) > 0);
	    break;

	  case 'v': verbose = atoi(optarg);
	    break;

	  case 'h': {
	    if ( Me == 0 )
	      printf( "\nvalid options are ( values btw [] are the default values ):\n"
		      "-x    x size of the plate [10000]\n"
		      "-y    y size of the plate [10000]\n"
		      "-e    how many energy sources on the plate [4]\n"
		      "-E    how many energy sources on the plate [1.0]\n"
		      "-n    how many iterations [1000]\n"
		      "-p    whether periodic boundaries applies  [0 = false]\n\n"
		      );
	    halt = 1; }
	    break;
	    
	    
	  case ':': printf( "option -%c requires an argument\n", optopt);
	    break;
	    
	  case '?': printf(" -------- help unavailable ----------\n");
	    break;
	  }
      }

    if ( opt == -1 )
      break;
  }

  if ( halt )
    return 1;
  
  
  // ··································································
  /*
   * here we should check for all the parms being meaningful
   *
   */

  // ...

  if ( (*S)[_x_] < 1 || (*S)[_y_] < 1 ) {
    if ( Me == 0 )
      printf("invalid size of the plate\n");
    return 2;
  }
  if ( *Nsources < 1 ) {
    if ( Me == 0 )
      printf("invalid number of heat sources\n");
    return 3;
  }
  if ( *Niterations < 1 ) {
    if ( Me == 0 )
      printf("invalid number of iterations\n");
    return 4;
  }
  if ( *energy_per_source <= 0.0 ) {
    if ( Me == 0 )
      printf("invalid energy per source\n");
    return 5;
  }
  if ( verbose < 0 ) {
    if ( Me == 0 )
      printf("invalid verbose level\n");
    return 6;
  }

  // ·

        

  
  // ··································································
  /*
   * find a suitable domain decomposition
   * very simple algorithm, you may want to
   * substitute it with a better one
   *
   * the plane Sx x Sy will be solved with a grid
   * of Nx x Ny MPI tasks
   */

  vec2_t Grid;
  double formfactor = ((*S)[_x_] >= (*S)[_y_] ? (double)(*S)[_x_]/(*S)[_y_] : (double)(*S)[_y_]/(*S)[_x_] );
  int    dimensions = 2 - (Ntasks <= ((int)formfactor+1) );

  
  if ( dimensions == 1 )
    {
      if ( (*S)[_x_] >= (*S)[_y_] )
	Grid[_x_] = Ntasks, Grid[_y_] = 1;
      else
	Grid[_x_] = 1, Grid[_y_] = Ntasks;
    }
  else
    {
      int   Nf;
      uint *factors;
      uint  first = 1;
      ret = simple_factorization( Ntasks, &Nf, &factors );
      
      for ( int i = 0; (i < Nf) && ((Ntasks/first)/first > formfactor); i++ )
	first *= factors[i];

      if ( (*S)[_x_] > (*S)[_y_] )
	Grid[_x_] = Ntasks/first, Grid[_y_] = first;
      else
	Grid[_x_] = first, Grid[_y_] = Ntasks/first;
    }

  (*N)[_x_] = Grid[_x_];
  (*N)[_y_] = Grid[_y_];
  

  // ··································································
  // my cooridnates in the grid of processors
  //
  int X = Me % Grid[_x_];
  int Y = Me / Grid[_x_];

  // ··································································
  // find my neighbours
  //

  if ( Grid[_x_] > 1 )
    {  
      if ( *periodic ) {       
	neighbours[EAST]  = Y*Grid[_x_] + (X + 1) % Grid[_x_];
	neighbours[WEST]  = Y*Grid[_x_] + (X - 1 + Grid[_x_]) % Grid[_x_]; }
      
      else {
	neighbours[EAST]  = ( X < Grid[_x_]-1 ? Me+1 : MPI_PROC_NULL );
	neighbours[WEST]  = ( X > 0 ? (Me-1)%Ntasks : MPI_PROC_NULL ); }  
    }

  if ( Grid[_y_] > 1 )
    {
      if ( *periodic ) {      
	neighbours[NORTH] = (Ntasks + Me - Grid[_x_]) % Ntasks;
	neighbours[SOUTH] = (Ntasks + Me + Grid[_x_]) % Ntasks; }

      else {    
	neighbours[NORTH] = ( Y > 0 ? Me - Grid[_x_]: MPI_PROC_NULL );
	neighbours[SOUTH] = ( Y < Grid[_y_]-1 ? Me + Grid[_x_] : MPI_PROC_NULL ); }
    }

  // ··································································
  // the size of my patch
  //

  /*
   * every MPI task determines the size sx x sy of its own domain
   * REMIND: the computational domain will be embedded into a frame
   *         that is (sx+2) x (sy+2)
   *         the outern frame will be used for halo communication or
   */
  
  vec2_t mysize;
  uint s = (*S)[_x_] / Grid[_x_];
  uint r = (*S)[_x_] % Grid[_x_];
  mysize[_x_] = s + (X < r);
  s = (*S)[_y_] / Grid[_y_];
  r = (*S)[_y_] % Grid[_y_];
  mysize[_y_] = s + (Y < r);

  planes[OLD].size[_x_] = mysize[_x_];
  planes[OLD].size[_y_] = mysize[_y_];
  planes[NEW].size[_x_] = mysize[_x_];
  planes[NEW].size[_y_] = mysize[_y_];
  

  if ( verbose > 0 )
    {
      if ( Me == 0 ) {
	printf("Tasks are decomposed in a grid %d x %d\n\n",
		 Grid[_x_], Grid[_y_] );
	fflush(stdout);
      }

      MPI_Barrier(*Comm);
      
      for ( int t = 0; t < Ntasks; t++ )
	{
	  if ( t == Me )
	    {
	      printf("Task %4d :: "
		     "\tgrid coordinates : %3d, %3d\n"
		     "\tneighbours: N %4d    E %4d    S %4d    W %4d\n",
		     Me, X, Y,
		     neighbours[NORTH], neighbours[EAST],
		     neighbours[SOUTH], neighbours[WEST] );
	      fflush(stdout);
	    }

	  MPI_Barrier(*Comm);
	}
      
    }

  
  // ··································································
  // allocae the needed memory
  //
  printf("[DEBUG] Llamando a memory_allocate...\n");
  printf("        'planes' ENVIADO con la dirección: %p\n", (void*)planes);
  printf("        'buffers' ENVIADO con la dirección: %p\n", (void*)buffers);
  fflush(stdout);
  ret = memory_allocate(neighbours, mysize, buffers, planes);
  
  if (ret)
  {
    printf("Task %d: memory_allocate failed with code %d. Aborting.\n", Me, ret);
    return ret; 
  }
  

  // ··································································
  // allocae the heat sources
  //
  ret = initialize_sources( Me, Ntasks, Comm, mysize, *Nsources, Nsources_local, Sources_local );
  
  if ( ret )
    {
      printf("task %d is opting out with termination code %d\n",
       Me, ret );
      
      memory_release( planes, buffers );
      return ret;
    }
  
  return 0;  
}


uint simple_factorization( uint A, int *Nfactors, uint **factors )
/*
 * rought factorization;
 * assumes that A is small, of the order of <~ 10^5 max,
 * since it represents the number of tasks
 #
 */
{
  int N = 0;
  int f = 2;
  uint _A_ = A;

  while ( f < A )
    {
      while( _A_ % f == 0 ) {
	N++;
	_A_ /= f; }

      f++;
    }

  *Nfactors = N;
  uint *_factors_ = (uint*)malloc( N * sizeof(uint) );

  N   = 0;
  f   = 2;
  _A_ = A;

  while ( f < A )
    {
      while( _A_ % f == 0 ) {
	_factors_[N++] = f;
	_A_ /= f; }
      f++;
    }

  *factors = _factors_;
  return 0;
}


int initialize_sources( int       Me,
			int       Ntasks,
			MPI_Comm *Comm,
			vec2_t    mysize,
			int       Nsources,
			int      *Nsources_local,
			vec2_t  **Sources )

{

  srand48(time(NULL) ^ Me);
  int *tasks_with_sources = (int*)malloc( Nsources * sizeof(int) );
  
  if ( Me == 0 )
    {
      for ( int i = 0; i < Nsources; i++ )
	tasks_with_sources[i] = (int)lrand48() % Ntasks;
    }
  
  MPI_Bcast( tasks_with_sources, Nsources, MPI_INT, 0, *Comm );

  int nlocal = 0;
  for ( int i = 0; i < Nsources; i++ )
    nlocal += (tasks_with_sources[i] == Me);
  *Nsources_local = nlocal;
  
  if ( nlocal > 0 )
    {
      vec2_t * restrict helper = (vec2_t*)malloc( nlocal * sizeof(vec2_t) );      
      for ( int s = 0; s < nlocal; s++ )
	{
	  helper[s][_x_] = 1 + lrand48() % mysize[_x_];
	  helper[s][_y_] = 1 + lrand48() % mysize[_y_];
	}

      *Sources = helper;
    }
  
  free( tasks_with_sources );

  return 0;
}



 int memory_release ( plane_t   *planes,
            buffers_t *buffers  
		     )
  
{

  if ( planes != NULL )
    {
        if ( planes[OLD].data != NULL )
          free (planes[OLD].data);
        
        if ( planes[NEW].data != NULL )
          free (planes[NEW].data);
    }

  if ( buffers != NULL )
    {
      for ( int b = 0; b < 2; b++ )
        // Only free EAST and WEST buffers, since NORTH and SOUTH are pointers into the plane data
        for ( int d = EAST; d <= WEST; d++ )
          if ( buffers[b][d] != NULL ) {
            free (buffers[b][d]);
          }
    }

      
  return 0;
}



int output_energy_stat ( int step, plane_t *plane, double budget, int Me, MPI_Comm Comm )
{

  double system_energy = 0;
  double tot_system_energy = 0;
  get_total_energy ( plane, &system_energy );
  
  MPI_Reduce ( &system_energy, &tot_system_energy, 1, MPI_DOUBLE, MPI_SUM, 0, Comm );
  
  if ( Me == 0 )
    {
      if ( step >= 0 )
	printf(" [ step %4d ] ", step ); fflush(stdout);

      
      printf( "total injected energy is %g, "
	      "system energy is %g "
	      "( in avg %g per grid point)\n",
	      budget,
	      tot_system_energy,
	      tot_system_energy / (plane->size[_x_]*plane->size[_y_]) );
    }
  
  return 0;
}
