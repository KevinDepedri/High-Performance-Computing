#!/bin/bash
#PBS -l select=2:ncpus=40:mem=2gb -l place=scatter

# Set max execution time
#PBS -l walltime=1:00:00

# Set the excution on the short queue
#PBS -q short_cpuQ

# Set name of job, output and error file
#PBS -N 80Scatter
#PBS -o 80Scatter.txt
#PBS -e 80Scatter_error.txt

# Load the library and execute the parallel application
module load mpich-3.2
mpiexec -n 80 /home/kevin.depedri/hpc3/runs/parallel/recursive/mpi_recursive_long /home/kevin.depedri/points/100M5d.txt -e -p