# --- Compilers ---
# Use mpicc, the MPI wrapper for the C compiler.
# On Leonardo, this will use the Intel compiler (icc/icx) or GCC
# depending on the loaded modules.
MPICC = mpicc

# --- Compilation Flags ---
# -O3: Maximum optimization level.
# -std=c11: Use the 2011 C standard.
# -Wall: Enable all compiler warnings for good practice.
CFLAGS = -O3 -std=c11 -Wall

# --- OpenMP Flags ---
# -fopenmp: Standard flag to enable OpenMP with GCC.
# For Intel compilers, this is typically -qopenmp, but the loaded
# Open MPI module is built with GCC, so -fopenmp is correct.
OMPFLAGS = -fopenmp

# --- Linker Flags ---
# -lm: Link the math library (for functions like sqrt).
LDFLAGS = -lm

# --- Files and Executable ---
# Final executable name
EXEC = stencil_parallel
# Source files (.c)
SRCS = stencil_template_parallel.c
# Object files (.o) to be generated
OBJS = $(SRCS:.c=.o)


# --- Makefile Rules ---

# Default rule: build the entire project.
all: $(EXEC)

# Rule to create the final executable
$(EXEC): $(OBJS)
	$(MPICC) $(OMPFLAGS) -o $(EXEC) $(OBJS) $(LDFLAGS)
	@echo "Executable '$(EXEC)' created successfully!"

# Rule to compile .c source files into .o object files
%.o: %.c
	$(MPICC) $(CFLAGS) $(OMPFLAGS) -c $< -o $@

# Rule to clean up generated files
clean:
	rm -f $(EXEC) $(OBJS) *.csv *.png *.out*
	@echo "Generated files cleaned up."

.PHONY: all clean