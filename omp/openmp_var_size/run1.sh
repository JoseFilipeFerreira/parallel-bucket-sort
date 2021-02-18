#!/bin/bash
cd "$PBS_O_WORKDIR"

echo -e "Starting job\n" >&2

module load gcc/7.2.0
module load python/3.8.2
echo "Sequential run" >&2
time ./bucket random.txt
echo "" >&2

echo "Sequential run v2" >&2
time ./bucket_seq_v2 random.txt
echo "" >&2

for i in 2 4 8 16 32 64; do
	echo "$i threads run" >&2
	export OMP_NUM_THREADS="$i"
	time ./bucket_omp random.txt;
	echo "" >&2
done

echo "Done!" >&2
