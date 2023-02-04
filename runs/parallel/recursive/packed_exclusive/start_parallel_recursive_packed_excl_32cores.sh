#!/bin/bash
#PBS -l select=2:ncpus=16:mem=2gb -l place=pack:excl

# Set max execution time
#PBS -l walltime=1:00:00

# Set the excution on the short queue
#PBS -q short_cpuQ

# Set name of job, output and error file
#PBS -N Rec32CorePackedExcl
#PBS -o Rec32CorePackedExcl.txt
#PBS -e Rec32CorePackedExcl_error.txt

# Load the library and execute the parallel application
module load mpich-3.2
mpiexec -n 32 /home/kevin.depedri/hpc3/runs/parallel/recursive/mpi_recursive_short_250M5d