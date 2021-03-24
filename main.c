/*
	FIT3143 Assignment 2
	
	Members:
	29906164 Sulthan De Neiro Raihan Syah Bungin 
	29458021 Nicholas Yeo Wei Ming

	This file contain the main program to run the entire system.

	NOTE: 
	PLEASE READ.MD IN THE FILE DIRECTORY TO RUN THE PROGRAM
	IF ERROR OCCURS, CLOSE THE TERMINAL AND REOPEN IT

	type "make" in the terminal to compile the program

	type "make run" in terminal to run the program

	type "make clean" in terminal to clean out the compiled program
*/

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <mpi.h>
#include <time.h>
#include "sensor.h"
#include "basestation.h"

/*
* Code that run the simulation as a whole
*/

int main(int argc,char **argv)
{
	// Show error to user if arguments is insufficient
	if(argc != 4){
		printf("\nArgument must be 3\n");
		printf("Arguments: [n] [m] [value of iteration (-1 for infinite)]\n");
		return 0;
	}

	// Allocate each arguments into variables
	int n = atoi(argv[1]);
	int m = atoi(argv[2]);
	int end = atoi(argv[3]);

	// Show error to user if arguments is incorrect
	if(n <= 0 || m <= 0 || end < -1){
		printf("\nArguments must be an integer an at least 1 and iteration must be at least -1\n");
		return 0;
	}

	// Allocate variables which will be used later
	int rank,size;
	int rank_master = n*m;
	int size_pack = 10000;
	int threshold = 80;
	int tolerance = 10;

	// Initialize MPI Environment
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Create 2 communicators, one for the global and one for the sensors only
	MPI_Comm comm;
	MPI_Comm_split(MPI_COMM_WORLD, rank_master == rank, rank, &comm);

	// Show error to user if number of processors allocated does not match with length and width
	if((n*m+1) != size){
		if(rank == 0){
			printf("Number of processors is not equal n x m + 1\n");
		}
		MPI_Finalize();
		return 0;
	}

	// If the rank of the process is the last (n*m), the process is the master
	if(rank == rank_master){
		
		master(MPI_COMM_WORLD, size, size_pack,n,m,threshold,tolerance);	
	}

	// Else, the process is the slave	
	else{

		slave(rank,size-1,size_pack,n,m,threshold,tolerance,MPI_COMM_WORLD, comm, end);
	}

	// Turn off global communicator and end MPI Environment
	MPI_Comm_free(&comm);
	MPI_Finalize();
	return 0;
}