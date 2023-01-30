// parallel mergesort try
/*
read all the points
divide by the number of cpus
each cpu sorts its part
each cpu sends the sorted part to the master
the master merges the parts
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/util.h"

#define AXIS 0
#define MASTER_PROCESS 0


void sendPointsPacked(Point* points, int numPoints, int destination, int tag, MPI_Comm comm) {
  int bufferSize = numPoints * (sizeof(int) + sizeof(int)*points[0].num_dimensions);
  void *buffer = malloc(bufferSize);
  int position = 0;

  for (int point = 0; point < numPoints; point++) {
    MPI_Pack(&points[point].num_dimensions, 1, MPI_INT, buffer, bufferSize, &position, comm);
    MPI_Pack(points[point].coordinates, points[point].num_dimensions, MPI_INT, buffer, bufferSize, &position, comm);
  }

  MPI_Send(buffer, position, MPI_PACKED, destination, tag, comm);
  free(buffer);
}

void recvPointsPacked(Point* points, int numPoints, int source, int tag, MPI_Comm comm) {
  MPI_Status status;
  MPI_Probe(source, tag, comm, &status);

  int count;
  MPI_Get_count(&status, MPI_PACKED, &count);
  void* buffer = malloc(count);

  MPI_Recv(buffer, count, MPI_PACKED, source, tag, comm, MPI_STATUS_IGNORE);

  int position = 0;
  for (int i = 0; i < numPoints; i++) {
    MPI_Unpack(buffer, count, &position, &points[i].num_dimensions, 1, MPI_INT, comm);
    points[i].coordinates = (int*)malloc(points[i].num_dimensions * sizeof(int));
    MPI_Unpack(buffer, count, &position, points[i].coordinates, points[i].num_dimensions, MPI_INT, comm);
  }
  free(buffer);
}


int main(int argc, char *argv[])
{
    int rank_process, comm_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_process);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    int num_points, num_dimensions;

    //OPTIONS: ../point_generator/points_channel_with_image/points_channel.txt   ../point_generator/10thousands.txt   ../point_generator/1milion.txt
    FILE *fp = fopen("../point_generator/points_channel_with_image/points_channel.txt", "r"); 
    if (fp == NULL)
    {
        printf("Error opening file on core %d", rank_process);
        return 1;
    }

    // Read the number of points and dimensions from the first line of the file
    fscanf(fp, "%d %d", &num_points, &num_dimensions);

    // Give error if the point are not enough
    if (num_points < 2)
    {
        printf("Error: the number of points must be greater than 1\n");
        return 1;
    }
    if (num_points < comm_size)
    {
        printf("Error: the number of points must be greater than the number of cores\n");
        return 1;
    }

    int num_points_normal_processes, num_points_local_process, points_in_excess, index_first_point, index_last_point = 0;

    // Compute the number of points that will be read by each core
    num_points_normal_processes = num_points / (comm_size-1);
    points_in_excess = num_points % (comm_size-1);
    printf("%d", num_points_normal_processes);
    
    if (rank_process != MASTER_PROCESS)
    {
        num_points_local_process = num_points / (comm_size-1);
        index_first_point = num_points_local_process * (rank_process-1);
        index_last_point = index_first_point + num_points_local_process - 1;
        printf("[NORMAL-PROCESS %d/%d] will read %d points from point %d to point %d\n", rank_process, comm_size, num_points_local_process, index_first_point, index_last_point);
    }
    else
    {
        num_points_local_process = num_points % (comm_size-1);
        index_first_point = num_points_normal_processes * (comm_size-1);
        if (num_points_local_process > 0)
            {index_last_point = index_first_point + num_points_local_process - 1;
            printf("[MASTER-PROCESS %d/%d] will read %d points from point %d to point %d\n", rank_process, comm_size, num_points_local_process, index_first_point, index_last_point);
        }
        else
        {
            printf("[MASTER-PROCESS %d/%d] will read %d points (point equally divided on the other cores)\n", rank_process, comm_size, num_points_local_process);
        }
    }

    // Compute the size of the header (number of points and number of dimensions)
    // long header_size = sizeof(num_points) + sizeof(num_dimensions);
    // printf("HEADER SIZE: %ld\n", header_size);

    // Jump to the point of the file which need to be read by this core
    long jump_size = 4 + 4 * 2 * (index_first_point); // The initial 4bytes is the size of the header (number of points and number of dimensions)
    if (fseek(fp, jump_size, SEEK_SET) != 0)
    {
        printf("Error: could not use fseek to perform jump inside the input point file\n");
    }

    // Allocate memory for the points in each core
    Point *local_process_points;
    local_process_points = (Point *)malloc(num_points_local_process * sizeof(Point));

    // Read the points and store them in this core
    for (int point = 0; point < num_points_local_process; point++)
    {
        local_process_points[point].num_dimensions = num_dimensions;
        local_process_points[point].coordinates = (int *)malloc(num_dimensions * sizeof(int));
        for (int dimension = 0; dimension < num_dimensions; dimension++)
        {
            fscanf(fp, "%d", &local_process_points[point].coordinates[dimension]);
        }
    }
    fclose(fp);
    

    // Print extracted points
    print_points(local_process_points, num_points_local_process, rank_process);  
    // Sort the points in each core
    mergeSort(local_process_points, num_points_local_process, AXIS);


    // Gather all the points in the main core
    if (rank_process == MASTER_PROCESS)
    {
        Point **processes_sorted_points;
        processes_sorted_points = (Point **)malloc(comm_size * sizeof(Point *));

        points_in_excess = num_points % comm_size;
        processes_sorted_points[0] = (Point *)malloc((num_points_local_process) * sizeof(Point));
        for (int process = 1; process < comm_size; process++)
        {
            processes_sorted_points[process] = (Point *)malloc(num_points_normal_processes * sizeof(Point));
        }

        // Save first the points ordered from the current core (core 0)
        processes_sorted_points[0] = local_process_points;
        for (int process = 1; process < comm_size; process++)
        {
            recvPointsPacked(processes_sorted_points[process], num_points_normal_processes, process, 0, MPI_COMM_WORLD);
        }
        printf("RECEIVE DONE\n");
        print_points(processes_sorted_points[0], num_points_local_process, rank_process); 
        print_points(processes_sorted_points[1], num_points_normal_processes, 1); 
        print_points(processes_sorted_points[2], num_points_normal_processes, 2); 
        printf("PRINT DONE\n");

        // Merge the points from all the cores
        Point *sorted_points;
        sorted_points = (Point *)malloc(num_points * sizeof(Point));
        int *temporary_indexes;
        temporary_indexes = (int *)malloc(comm_size * sizeof(int));
        for (int process = 0; process < comm_size; process++)
        {
            temporary_indexes[process] = 0;
        }
        for (int i = 0; i < num_points; i++)
        {
            int min = processes_sorted_points[0][temporary_indexes[0]].coordinates[AXIS];
            int process_with_minimum_value = 0;
            for (int process = 0; process < comm_size; process++)
            {
                if ((process < comm_size && temporary_indexes[process] < num_points_local_process) || (process == 0 && temporary_indexes[process] < num_points_local_process + points_in_excess))
                {
                    if (processes_sorted_points[process][temporary_indexes[process]].coordinates[AXIS] < min)
                    {
                        min = processes_sorted_points[process][temporary_indexes[process]].coordinates[AXIS];
                        process_with_minimum_value = process;
                    }
                }
            }
            sorted_points[i] = processes_sorted_points[process_with_minimum_value][temporary_indexes[process_with_minimum_value]];
            temporary_indexes[process_with_minimum_value]++;
        }

        // Print the sorted points ignoring verbose
        for (int point = 0; point < num_points; point++)
        {
            printf("Point %d: (", point);
            for (int dimension = 0; dimension < sorted_points[point].num_dimensions; dimension++)
            {
                if (dimension != sorted_points[point].num_dimensions - 1)
                {
                    printf("%d, ", sorted_points[point].coordinates[dimension]);
                }
                else
                {
                    printf("%d)\n", sorted_points[point].coordinates[dimension]);
                }
            }
        }

    }
    else
    {
        // All the other cores send their ordered points to core 0
        sendPointsPacked(local_process_points, num_points_local_process, 0, 0, MPI_COMM_WORLD);
        printf("SEND DONE\n");
    }

    MPI_Finalize();
    return 0;
}