#!/bin/bash
#PBS -l select=2:ncpus=4:mem=2gb -l place=scatter

# Set max execution time
#PBS -l walltime=3:00:00

# Set the excution on the short queue
#PBS -q short_cpuQ

# Set name of job, output and error file
#PBS -N Rec8CoreScatter
#PBS -o Rec8CoreScatter.txt
#PBS -e Rec8CoreScatter_error.txt

# Load the library and execute the parallel application
module load mpich-3.2
mpiexec -n 8 /home/kevin.depedri/hpc3/runs/parallel/recursive/mpi_recursive_short_250M5d