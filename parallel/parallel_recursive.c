#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils/util.h"

#define AXIS 0
#define MASTER_PROCESS 0
#define INT_MAX 2147483647


int main(int argc, char *argv[])
{
    // clock_t start, end;
    // double cpu_time_used;
    // start = clock();

    int verbose = 0;

    MPI_Init(&argc, &argv);
    int rank_process, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_process);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    char path[] = "../point_generator/1hundred.txt";

    int num_points, num_dimensions;

    if (rank_process == MASTER_PROCESS)
    {
        // Open input point file on master process
        FILE *point_file = fopen(path, "r"); 
        if (point_file == NULL)
        {
            perror("Error opening file on master process\n");
            return -1;    
        }

        // Read the number of points and dimensions from the first line of the file
        fscanf(point_file, "%d %d", &num_points, &num_dimensions);

        // Give error if the points or processes are not enough
        if (num_points < 2)
        {
            perror("Error: the number of points must be greater than 1\n");
            return -1;
        }
        if (num_points < comm_size)
        {
            perror("Error: the number of points must be greater than the number of processes\n");
            return -1;
        }
        if (comm_size < 2)
        {
            perror("Error: cannot run the parallel program over just 1 process\n");
            return -1;
        }
        fclose(point_file);
    }
    // Send num_points to all the processes, it is used to compute the num_points_...
    MPI_Bcast(&num_points, 1, MPI_INT, MASTER_PROCESS, MPI_COMM_WORLD);
    MPI_Bcast(&num_dimensions, 1, MPI_INT, MASTER_PROCESS, MPI_COMM_WORLD);

    // Points are divided equally on all processes exept master process which takes the remaing points
    int num_points_normal_processes, num_points_master_process, num_points_local_process;
    num_points_normal_processes = num_points / (comm_size-1);
    num_points_master_process = num_points % (comm_size-1);
    Point *local_points;

    Point *all_points = NULL;
    all_points = parallel_order_points(all_points, path, rank_process, comm_size, verbose);
    if (rank_process == MASTER_PROCESS)
    {
        if (all_points == NULL)
        {
            perror("Error in executing parallel_order_points\n");
            return -1;
        }
    }



    MPI_Barrier(MPI_COMM_WORLD);
    /* IMPLEMENT FROM HERE 

    1. DIVIDE POINTS OVER PROCESSES + 2 EXTRA COORDINATES FOR EACH PROCESS (used to compute boundary). PROCESS 1 AND LAST ONE (master) WILL HAVE JUST 1 COORDINATE
    2. SEND POINTS
    3. COMPUTE MIN DISTANCE FOR EACH PROCESS
    4. ALLREDUCE MIN DISTANCE THROUGH MASTER PROCESS AND SEND IT BACK TO ALL PROCESSES
    5. COPUTE BOUNDARY BETWEEN PROCESSES
    6. GET POINTS INTO THE LEFT STRIP USING THE COMMON MIN-DISTANCE (x_i - x_0 < min_distance)
    7. SEND POINT OF THE LEFT STRIP TO THE LEFT PROCESS
    8. EACH PROCESS EXCEPT PROCESS 0(last one) BUILD ITS RIGHT STRIP AND MERGE IT WITH THE RECEIVED LEFT STRIP
    9. REORDER POINTS IN THE STRIP OVER Y
    10. COMPUTE MIN DISTANCE FOR EACH STRIP (line 44 of sequential_recursive)
    11. REDUCE ALL DISTANCES BACK TO MASTER PROCESS
    12. RETURN MIN DISTANCE
    */

    // POINT 1 AND 2 - divide points and send them with thei out_of_region valie
    int left_x_out_of_region = INT_MAX, right_x_out_of_region = INT_MAX;
    if (rank_process == MASTER_PROCESS)
    {
        // Transfer total number of points and the correct slice of points to work on for every process
        Point *points_to_send;
        points_to_send = (Point *)malloc(num_points_normal_processes * sizeof(Point));
        int left_to_send, right_to_send;
        for (int process = 1; process < comm_size; process++)
        {
            // TODO: eventually send master process #points
            for (int point = 0; point < num_points_normal_processes; point++)
            {
                points_to_send[point].num_dimensions = num_dimensions;
                points_to_send[point].coordinates = (int *)malloc(num_dimensions * sizeof(int));
                for (int dimension = 0; dimension < num_dimensions; dimension++)
                {
                    points_to_send[point].coordinates[dimension] = all_points[point+num_points_normal_processes*(process-1)].coordinates[dimension];
                }
            }
            MPI_Send(&num_points_normal_processes, 1, MPI_INT, process, 0, MPI_COMM_WORLD);
            sendPointsPacked(points_to_send, num_points_normal_processes, process, 1, MPI_COMM_WORLD);
            
            // Transfer also the left and right out_of_region x coordinate for each process
            if (process == 1){
                left_to_send = INT_MAX;
                right_to_send = all_points[num_points_normal_processes*(process)].coordinates[AXIS];
            }
            else{
                left_to_send = all_points[num_points_normal_processes*(process-1)-1].coordinates[AXIS]; 
                if (process != comm_size - 1 || num_points_master_process != 0) //rank_process != 0 && (rank_process != comm_size - 1 || num_points_master_process != 0)
                    right_to_send = all_points[num_points_normal_processes*(process)].coordinates[AXIS];
                else
                    right_to_send = INT_MAX;
            }
            MPI_Send(&left_to_send, 1, MPI_INT, process, 2, MPI_COMM_WORLD);
            MPI_Send(&right_to_send, 1, MPI_INT, process, 3, MPI_COMM_WORLD);
        }

        // Free points_to_send once the send for all the processes is done 
        for (int point = 0; point < num_points_normal_processes; point++)
        {
            free(points_to_send[point].coordinates);
        }
        free(points_to_send);

        // Store the points of the master process and its out_of_region values
        num_points_local_process = num_points_master_process;
        local_points = (Point *)malloc(num_points_local_process * sizeof(Point));
        int starting_from = num_points_normal_processes * (comm_size - 1);
        for (int i = 0; i + starting_from < num_points; i++)
        {
            local_points[i].num_dimensions = num_dimensions;
            local_points[i].coordinates = (int *)malloc(num_dimensions * sizeof(int));
            for (int dimension = 0; dimension < num_dimensions; dimension++)
            {
                local_points[i].coordinates[dimension] = all_points[i + starting_from].coordinates[dimension];
            }
        }
        left_x_out_of_region = all_points[starting_from-1].coordinates[AXIS];
        right_x_out_of_region = INT_MAX;
    }
    else
    {
        // Receive number of points and the points data that will be processed by this process
        MPI_Recv(&num_points_normal_processes, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        num_points_local_process = num_points_normal_processes;
        local_points = (Point *)malloc(num_points_local_process * sizeof(Point));
        recvPointsPacked(local_points, num_points_local_process, 0, 1, MPI_COMM_WORLD);

        // Receive the left and right out_of_region x coordinate for this process
        MPI_Recv(&left_x_out_of_region, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&right_x_out_of_region, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    // print_points(local_points, num_points_local_process, rank_process);

    // POINT 3 - compute min distance for eahc process
    double local_dmin = INT_MAX;
    if (num_points_local_process > 1)
        local_dmin = recSplit(local_points, num_points_local_process);
    // printf("PROCESS:%d LEFT OUT VALUE: %d, RIGHT OUT VALUE: %d, DMIN:%f\n", rank_process, left_x_out_of_region, right_x_out_of_region, local_dmin);

    // POINT 4 - allreduce to find the global dmin
    double global_dmin;
    MPI_Allreduce(&local_dmin, &global_dmin, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
    if (rank_process == MASTER_PROCESS)
        printf("GLOBAL DMIN: %f\n", global_dmin);
    
    // POINT 5 - compute the boundary between processes
    double left_boundary = INT_MAX, right_boundary = INT_MAX;

    if (rank_process != 1 && num_points_local_process != 0)
        left_boundary = (left_x_out_of_region + local_points[0].coordinates[AXIS])/2.0;
    // printf("\nPROCESS: %d, VALORE: %d\n",rank_process, num_points_master_process);

    if(rank_process != 0 && (rank_process != comm_size - 1 || num_points_master_process != 0))
        right_boundary = (right_x_out_of_region + local_points[num_points_local_process-1].coordinates[AXIS])/2.0;
    printf("PROCESS:%d LEFT BOUNDARY: %f, RIGHT BOUNDARY: %f\n\n", rank_process, left_boundary, right_boundary);
    
    // POINT 5 - get the points in the strips for each process
    // get for both left and write side, points where x_i - dmin <= boundary
    int num_points_left_strip = 0, num_points_right_strip = 0;
    Point *left_strip_points = (Point *)malloc(num_points_local_process * sizeof(Point));
    Point *right_strip_points = (Point *)malloc(num_points_local_process * sizeof(Point));
    // TODO: IMPROVEMENT: USE 2 FOR LOOPS IN THE TWO DIRECTION AND BREAK AS SOON AS A VALUE IS OUT OF DELTA
    for (int point = 0; point < num_points_local_process; point++)
    {
        if (local_points[point].coordinates[AXIS] < left_boundary + global_dmin)
        {
            left_strip_points[num_points_left_strip].num_dimensions = num_dimensions;
            left_strip_points[num_points_left_strip].coordinates = (int *)malloc(num_dimensions * sizeof(int));
            for (int dimension = 0; dimension < num_dimensions; dimension++)
            {
                left_strip_points[num_points_left_strip].coordinates[dimension] = local_points[point].coordinates[dimension];
            }
            num_points_left_strip++;
        }
        if (local_points[point].coordinates[AXIS] > right_boundary - global_dmin)
        {
            right_strip_points[num_points_right_strip].num_dimensions = num_dimensions;
            right_strip_points[num_points_right_strip].coordinates = (int *)malloc(num_dimensions * sizeof(int));
            for (int dimension = 0; dimension < num_dimensions; dimension++)
            {
                right_strip_points[num_points_right_strip].coordinates[dimension] = local_points[point].coordinates[dimension];
            }
            num_points_right_strip++;
        }
    }
    printf("PROCESS: %d, LEFT: %d, RIGHT: %d\n",rank_process, num_points_left_strip, num_points_right_strip);

    // POINT 6 - move the point to the left process if process not == 1 nor if the 0th process hasn't any points
    // if (rank_process != 1 && rank_process != 0)
    // {
    //     // Send the number of points to the right process
    //     MPI_Send(&num_points_right_strip, 1, MPI_INT, rank_process+1, 0, MPI_COMM_WORLD);
    //     // Send the points to the right process
    //     sendPointsPacked(right_strip_points, num_points_right_strip, rank_process+1, 1, MPI_COMM_WORLD);
    // }


    // END OF IMPLEMENTATION
    if (rank_process == MASTER_PROCESS)
    {
        printf("ORDERED POINTS:\n");
        print_points(all_points, num_points, rank_process);

        // Free all points
        for (int point = 0; point < num_points; point++)
        {
            free(all_points[point].coordinates);
        }
    }
    free(all_points);

    // if (rank_process == MASTER_PROCESS)
    // {
        // printf("Memory free\n");
        // end = clock();
        // cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        // printf("Time elapsed: %f\n", cpu_time_used);
    // }

    MPI_Finalize();
    return 0;
}