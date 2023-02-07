#!/bin/bash
#PBS -l select=2:ncpus=8:mem=2gb -l place=pack:excl

# Set max execution time
#PBS -l walltime=1:00:00

# Set the excution on the short queue
#PBS -q short_cpuQ

# Set name of job, output and error file
#PBS -N Rec16CorePackedExcl
#PBS -o Rec16CorePackedExcl.txt
#PBS -e Rec16CorePackedExcl_error.txt

# Load the library and execute the parallel application
module load mpich-3.2
mpiexec -n 16 /home/kevin.depedri/hpc3/runs/parallel/recursive/mpi_recursive_short /home/kevin.depedri/points/250M5d.txt -e -p