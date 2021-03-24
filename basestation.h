/* 
*Note .h files describe what the code does, .c files describe how the code works
*/
#define _basestation_h

#include <stdio.h>
#include <mpi.h>

/* 
* Description: 
* 
* Input: 
* @MPI_Comm comm_all :Communicator grouping all the threads
* @int size :the number of MPI threads
* @int sizepack :the the size of the mpi pack
* @int n :the length of the matrix of sensors
* @int m :the height of the matrix of sensors
* @int threshold :the temperature where an alert is created
* @int tolerance :range of error allowed for temperatures 
*/
int master(MPI_Comm comm_all,int size, int sizePack, int n, int m,int threshold, int tolerance);

/* 
* Description: 
* 
* Input: 
* @double time_start: the time where 
* @compare: time to compare
*/
int checkEnd(double time_start,double compare);