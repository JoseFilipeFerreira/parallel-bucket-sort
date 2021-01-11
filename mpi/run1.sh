#!/bin/bash
cd "$PBS_O_WORKDIR"

echo -e "Starting job\n" >&2

module load gcc/7.2.0
echo "Sequential run" >&2
time mpirun -np 10 ./a.out ../bin/random.txt
echo "" >&2


echo "Done!" >&2
