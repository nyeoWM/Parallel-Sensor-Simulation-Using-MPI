/*
	FIT3143 Assignment 2
	
	Members:
	29906164 Sulthan De Neiro Raihan Syah Bungin 
	29458021 Nicholas Yeo Wei Ming

	This file contain the program to run the sensor nodes.
*/

/* 
*Note .h files describe what the code does, .c files describe how the code works
*/
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <time.h>
#include <sys/time.h>
#include <mpi.h>
#include <unistd.h>
#include <math.h>
#include "sensor.h"
#include "basestation.h"

int slave(int rank, int size, int sizePack, int n, int m, int threshold, int tolerance, MPI_Comm comm_all, MPI_Comm comm_sensor, int end)
{
	// Initiate local communicator 
	MPI_Comm comm_wsn;
	MPI_Status status;

	// Initally: left,right,top,bottom
	int dir[4];
	int dims[2], coord[2], wrap[2];

	// Initialize local ranks
	int rank_local;

	// Initiate variable for creating dimension later
	dims[0] = n;
	dims[1] = m;
	wrap[0] = wrap[1] = 0;

	// Create MPI Cart Dimension
	MPI_Dims_create(size, 2, dims);
	MPI_Cart_create(comm_sensor, 2, dims, wrap, 1, &comm_wsn);
	
	// Allocate local rank for local communicator
	MPI_Comm_rank(comm_wsn, &rank_local);

	// Coordinates and Rank for local communicator    
	MPI_Cart_coords(comm_wsn, rank_local, 2, coord);
	MPI_Cart_rank(comm_wsn, coord, &rank_local);

	// Initiate variables for iteration later 
	int run = 0;
	int notified = 0;
	int infinite = 0;

	// Initiate variables for holding the value of IP address later
	char hostbuffer[256],IPEndbuffer[20]; 
    char *IPbuffer; 
    struct hostent *host_entry; 
    int hostname; 

	// Get the IP address of the current rank and convert the buffer string into string
	hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
	host_entry = gethostbyname(hostbuffer); 
	IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); 
	strncpy(IPEndbuffer,IPbuffer,20);

	// If the end value is -1, then the iteration of the sensor node will be infinite
	if(end == -1){
		infinite = 1;
	}

	// The main while loop that will iterate until reach the end value or until it being notified by the base station to stop
	while (run < end || infinite == 1)
	{	
		
		MPI_Barrier(comm_wsn);

		//printf(">>> %d %d\n",rank,run);

		int flag = 0;

		// Check the timing to determine if the sensor nodes should be terminated or not
		double delay = 0.1 * CLOCKS_PER_SEC; 
	  	delay = ceil(delay);
	    clock_t start = clock(); 
	  	clock_t endT = start + delay;

		// Check for termination message from the base station
    	while (clock() < endT && flag == 0){
			MPI_Iprobe(n*m, 24, comm_all, &flag, &status);
		}

		// If there are termination message, terminate the main while loop 
		if (flag != 0){
			MPI_Recv(&notified, 1, MPI_INT, n*m, 24, comm_all, &status);
			if(notified == 2){
				printf(">>> %d \n",rank_local);
				break;
			}
		}

		// Increment the value
		run += 1;

		// Initialize variable for counting comparison and holding adjacent values
		int match = 0;
		int adj[4] = {-1, -1, -1, -1};
		int adj_x[4] = {-1, -1, -1, -1};
		int adj_y[4] = {-1, -1, -1, -1};
		int adj_temperature[4] = {-1, -1, -1, -1};
		char adj_ip[4][20];

		// Check the top and left of the current node/process
		MPI_Cart_shift(comm_wsn, 0, 1, &dir[2], &dir[3]);
		MPI_Cart_shift(comm_wsn, 1, 1, &dir[0], &dir[1]);

		// Generate random temperature for the current node/process
		int temperature = generateRandom(run + rank_local);
		int temperatureCompare = -1;

		// Send the current process's temperature to adjacent nodes
		for (int i = 0; i < 4; i++)
		{
			if (dir[i] != -1)
			{
				MPI_Send(&temperature, 1, MPI_INT, dir[i], 23, comm_wsn);

			}
		}

		// Check the bottom and right of the current node/process
		MPI_Cart_shift(comm_wsn, 0, -1, &dir[2], &dir[3]);
		MPI_Cart_shift(comm_wsn, 1, -1, &dir[0], &dir[1]);

		// Receive the temperature from adjacent nodes
		for (int i = 0; i < 4; i++)
		{
			int flagRec = 0;
			int termiRec = 0;

			double timeRec = MPI_Wtime();
			while(flagRec == 0 && termiRec == 0){
				MPI_Iprobe(MPI_ANY_SOURCE, 23, comm_wsn, &flagRec, &status);
				termiRec = checkEnd(timeRec,2);
			}
			MPI_Recv(&temperatureCompare, 1, MPI_INT, dir[i], 23, comm_wsn, &status);

			// If the temperature is above threshold, under the range of the tolerance and exist
			if (temperature > threshold)
			{
				if (temperatureCompare != -1)
				{
					if (temperatureCompare >= temperature - tolerance && temperatureCompare <= temperature + tolerance)
					{
						// Store the values in the allocated arrays 
						adj[i] = dir[i];
						adj_temperature[i] = temperatureCompare;
						hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
						host_entry = gethostbyname(hostbuffer); 
						IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); 
						strncpy(IPEndbuffer,IPbuffer,20);
						strncpy(adj_ip[i],IPEndbuffer,20);

						// Increment the match counter, implies that there is adjacent node that fulfill the requirement
						match += 1;
					}
				}
			}

			// Reset the value for future comparison
			temperatureCompare = -1;

			// Store the coords of the adjacent nodes inside the allocated array
			if (adj[i] != -1)
			{
				if (i == 0)
				{
					adj_x[i] = coord[0];
					adj_y[i] = coord[1] + 1;
				}
				else if (i == 1)
				{
					adj_x[i] = coord[0];
					adj_y[i] = coord[1] - 1;
				}
				else if (i == 2)
				{
					adj_x[i] = coord[0] + 1;
					adj_y[i] = coord[1];
				}
				else if (i == 3)
				{
					adj_x[i] = coord[0] - 1;
					adj_y[i] = coord[1];
				}
			}
		}

		// Initiate variable for MPI_Pack later
		int posBase = 0;
		char packBase[sizePack];

		// If there are 2 or more adjacent nodes with similarity temperature
		if (match >= 2)
		{
			// CGet timestamp for event logging and message sending
			time_t timeCheck = time(NULL);
			struct tm t = *localtime(&timeCheck);

			struct timeval time_send;
			gettimeofday(&time_send, NULL);

			// Split the time structure into seperate integers for Packing later
			int t_year = t.tm_year + 1900;
			int t_month = t.tm_mon + 1;
			int t_day = t.tm_mday;
			int t_day_name = t.tm_wday;
			int t_hour = t.tm_hour;
			int t_min = t.tm_min;
			int t_sec = t.tm_sec;

			// Pack all the information 
			MPI_Pack(&run, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&match, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);

			MPI_Pack(&temperature, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&rank, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&coord[0], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&coord[1], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&IPEndbuffer, 20, MPI_CHAR, packBase, sizePack, &posBase,comm_all);

			for (int j = 0; j < 4; j++)
			{
				MPI_Pack(&adj_temperature[j], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
				MPI_Pack(&adj[j], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
				MPI_Pack(&adj_x[j], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
				MPI_Pack(&adj_y[j], 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
				MPI_Pack(&adj_ip[j], 20, MPI_CHAR, packBase, sizePack, &posBase,comm_all);
			}

			MPI_Pack(&t_year, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_month, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_day, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_day_name, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_hour, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_min, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&t_sec, 1, MPI_INT, packBase, sizePack, &posBase, comm_all);
			MPI_Pack(&time_send.tv_usec, 1, MPI_LONG, packBase, sizePack, &posBase, comm_all);

			// Send the message contain the packed information to the base station
			MPI_Send(packBase, sizePack, MPI_PACKED, n * m, 23, comm_all);

		}

	}

	// Terminate local communicator
	MPI_Comm_free(&comm_wsn);
	return 0;
}

int generateRandom(int value)
{
	// Get timestamp to increase variety on seed generation for randomization
	struct timeval time_val;
	gettimeofday(&time_val, NULL);

	// Improve randomization based on the input value of the rank and timing 
	int added = (int) (time_val.tv_usec * 1000000);
	srand(value+added);

	int valueRandom = rand() % 30;

	return valueRandom+70;
}