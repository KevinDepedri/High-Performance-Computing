echo 'COMPILE'
mpicc -std=c99 -g -Wall -o mpi_recursive3 parallel_recursive3.c utils/util.c -lm
echo 'PARALLEL RUN 6 CORES'
mpiexec -n 6 mpi_recursive3
echo '--------------------------------------------------------------------------'
echo 'PARALLEL RUN 5 CORES'
mpiexec -n 5 mpi_recursive3
echo '--------------------------------------------------------------------------'
echo 'SEQUENTIAL RUN 4 CORES'
mpiexec -n 4 mpi_recursive3
echo '--------------------------------------------------------------------------'
echo 'PARALLEL RUN 3 CORES'
mpiexec -n 4 mpi_recursive3
echo '--------------------------------------------------------------------------'
echo 'PARALLEL RUN 2 CORES'
mpiexec -n 3 mpi_recursive3
echo '--------------------------------------------------------------------------'
echo 'SEQUENTIAL RUN 1 CORES'
mpiexec -n 2 mpi_recursive3
echo '--------------------------------------------------------------------------'