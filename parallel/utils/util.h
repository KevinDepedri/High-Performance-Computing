typedef struct{
    int *coordinates;
    int num_dimensions;
} Point;

extern double distance(Point, Point);

extern void merge(Point *, int, int, int, int);

extern void mergeSortRec(Point *, int, int, int);

extern void mergeSort(Point *, int, int);

extern void print_points(Point *, int, int);

extern void sendPointsPacked(Point*, int, int, int, MPI_Comm);

extern void recvPointsPacked(Point*, int, int, int, MPI_Comm);