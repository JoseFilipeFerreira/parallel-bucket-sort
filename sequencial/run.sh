#!/bin/bash
echo -e "Starting job\n"

cd "$PBS_O_WORKDIR"

module load gcc/5.3.0

echo "Compiling..."
make

echo -n "Running tests...\n"

for min in seq -100 -10 -500
do
    for max in seq 100 10 500
    do
        for size in seq 100 100 1000
        do
            echo "Running test for random min:$min max:$max size:$size"
            ./generate "$min" "$max" "$size" random.txt
            result="$(./bucket random.txt)"
            correct="$(cat random.txt | tr " " "\n" | sort -g | tr "\n" " ")"
            if [[ $result == $correct ]]
            then
                echo -e ">\e[32mCorrect\e[0m\n"
            else
                echo -e ">\e[31mWrong\e0m\n"
            fi
        done
    done
done

echo "Cleaning..."
make clean

echo "Done!"
