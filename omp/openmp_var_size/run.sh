#!/bin/bash
cd "$PBS_O_WORKDIR"

echo -e "Starting job\n"

module load gcc/7.2.0
module load python/3.8.2

echo "Compiling..."
make

echo -n "Running tests...\n"

for min in `seq -100 -10 -500`
do
    for max in `seq 100 10 500`
    do
        for size in `seq 100 100 1000`
        do
            echo "Running test for random min:$min max:$max size:$size"
            python generate "$min" "$max" "$size" random.txt
            result="$(./bucket random.txt)"
            correct="$(sort -g random.txt)"
            if [[ $result == $correct ]]
            then
                echo -e ">\e[32mCorrect\e[0m"
            else
                echo -e ">\e[31mWrong\e[0m"
                exit
            fi
        done
    done
done

echo ""
echo "Cleaning..."
make clean

echo "Done!"
