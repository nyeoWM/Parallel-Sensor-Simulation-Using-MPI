/* 
*Note .h files describe what the code does, .c files describe how the code works
*/
#define _sensor_h

#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <mpi.h>

/* 
* Description: 
* 

* Input: 
* @int rank :the rank of the current thread
* @int size :the number of MPI threads
* @int sizepack :the the size of the mpi pack
* @int n :the length of the matrix of sensors
* @int m :the height of the matrix of sensors
* @int threshold :the temperature where an alert is created
* @int tolerance :range of error allowed for temperatures 
* @MPI_Comm comm_all :Communicator grouping all the threads
* @MPI_Comm comm_sensor :Communicator consist of 
* @int end :The maximum iteration, if -1, the iteration is infinite  
*/
int slave(int rank, int size, int sizePack, int n,int m, int threshold,int tolerance,MPI_Comm comm_all, MPI_Comm comm_sensor, int end);

int generateRandom(int value);
