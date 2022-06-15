#!/bin/bash
# Configure the resources required
#SBATCH -n 1                # number of cores
#SBATCH --time=01:00:00     # time allocation, which has the format DD:HH:MM
#SBATCH --gres=gpu:1        # generic resource required (1 GPU)

echo "Serial Algorithm"
echo "Array Size: $1" 

for i in {1..10}
do
	./serial "$1"
done

echo ""