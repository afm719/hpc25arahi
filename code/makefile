# Makefile to replicate a specific command, meant to be run from the 'code' directory


MPICC = mpicc #compiler for parallel code


CFLAGS = -03 -march=native -Iinclude 

OMPFLAGS = -fopenmp


EXEC = stencil_parallel
SRCS = src/stencil_template_parallel.c

all:
	$(MPICC) $(CFLAGS) $(OMPFLAGS) -o $(EXEC) $(SRCS)
	@echo "Executable '$(EXEC)' created successfully with your flags (-g -O0)."

clean:
	rm -f $(EXEC)
	@echo "Executable cleaned up."

.PHONY: all clean