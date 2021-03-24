/*	
Credits to
Sulthan De Neiro Raihan Syah Bungin 
Nicholas Yeo Wei Ming

	This file contain the program to run the base station.
*/

/* 
*Note .h files describe what the code does, .c files describe how the code works
*/
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>
#include <unistd.h>
#include "sensor.h"
#include "basestation.h"

/*
* Runs in a constant loop until the user quits, periodically checking for messages from the sensors. 
* When a message from a sensor is received, the base station will  
*
* To quit, user inputs 10 into input txt file. 
*/

int master(MPI_Comm comm_all, int size, int sizePack, int n, int m, int threshold, int tolerance)
{

    // Initialize the variables that handle MPI_Pack 
    char pack[sizePack];
    int pos;
    const char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    // Initialize variable that will be packed inside the MPI_Pack
    int temperature, rank, match;
    int coord_x, coord_y;
    int t_year, t_month, t_day, t_day_name, t_hour, t_min, t_sec;
    long t_usec;

    // Alert Checking
    int alert = 0;

    // For Communication time
    double time_com;

    // Allocated for holding the values of the adjacent nodes
    int adj[4] = {-1, -1, -1, -1};
    int adj_x[4] = {-1, -1, -1, -1};
    int adj_y[4] = {-1, -1, -1, -1};
    int adj_temperature[4] = {-1, -1, -1, -1};
    char adj_ip[4][20];

    // Variables that will handle iteration and termination
    int terminate = 0;
    int terminate_notify = 1;
    int run;

    // Variable to check if there are no more message being received within the time period
    double time_start;

    // Variable Allocations for IP address
    char current_ip[20];
    char ch[10];

    // Variable for handling manual user termination
    int stop;

    // Allocation for output information into text file
    FILE *fp, *fout;
    fout = fopen("outputStation.txt", "w");

    // OpenMP number of threads allocated to be used for satellite data generation
    omp_set_num_threads(n * m);

    // Global array allocation for holding the satellite temperature and timestamp
    int satellite_temperature[n][m];
    struct tm satellite_time[n][m];

    // Data structures for getting time
    time_t timeCheck = time(NULL);

    // Loop that will iterate until there are no more messages receive from sensor nodes or terminated by user manually
    while (1)
    {
        
        // Run in 100 ms per iteration
        sleep(0.1);

        // Checking when the logged time started
        time_t time_log = time(NULL);
        struct tm log = *localtime(&time_log);

        // Checking the input file that function as manual termination
        fp = fopen("inputStation.txt", "r");
        fscanf(fp, "%d", &stop);

        // Under the assumption that termination does not happen
        terminate_notify = 1;

        // If the value in the text file is not 1, continue
        if (stop != 1)
        {

        }

        // If the value in text file is 1, do termination process
        else
        {
            // Change the value of termination and send termination notifications to all sensor nodes
            terminate_notify = 2;

            #pragma omp parallel
            {
                #pragma omp for
                for (int k = 0; k < n * m; k++)
                {
                    MPI_Send(&terminate_notify, 1, MPI_INT, k, 24, comm_all);
                }
            }

            // Terminate the main while loop inside the base station 
            terminate = 1;

            printf("\nTerminated by user input (text file has value 1)\n");
            break;
            
        }

        // Close the file
        fclose(fp);

        // Prepare to receive the 
        pos = 0;
        int flag = 0;
        MPI_Status status;

        // Record time for checking how long the base station waiting for messages
        time_start = MPI_Wtime();

        // Loop until either message being receive or it goes beyond the time needed
        while (flag == 0 && terminate_notify != 2)
        {   
            // Check if there are message
            MPI_Iprobe(MPI_ANY_SOURCE, 23, comm_all, &flag, &status);
            
            // if beyond 2 seconds there are no messages, do termination process
            terminate = checkEnd(time_start,3);

            // Change the value of termination and send termination notifications to all sensor nodes
            if (terminate == 1)
            {
                flag = 1;
                terminate_notify = 2;

                for (int k = 0; k < n * m; k++)
                {
                    MPI_Send(&terminate_notify, 1, MPI_INT, k, 24, comm_all);
                }

                printf("\nTerminated by iteration limit\n");
            }
        }

        // Terminate the main while loop inside the base station  
        if (terminate == 1)
        {
            break;
        }
        /**************************************************************/
        /* Code for infrared satallite that runs on a seperate thread */
        /**************************************************************/

        // Start parallelism for satellite data generation 
        #pragma omp parallel
        {
            #pragma omp for collapse(2)
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < m; j++)
                {
                    // Satellite temperature and timestamp generation that stored in the global array
                    satellite_temperature[i][j] = generateRandom(i * 2 + j);
                    satellite_time[i][j] = *localtime(&timeCheck);
                }
            }
        }
        /*End of satellite code****************************************/
        /**************************************************************/

        // Receiving the message from the sensor nodes
        MPI_Recv(pack, sizePack, MPI_PACKED, MPI_ANY_SOURCE, 23, comm_all, &status);

        // Get the current time to calculate the communication time later
        struct timeval time_rec;
        gettimeofday(&time_rec, NULL);
        double time_receive = time_rec.tv_usec;

        // Unpack the data inside the message
        MPI_Unpack(pack, sizePack, &pos, &run, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &match, 1, MPI_INT, comm_all);

        MPI_Unpack(pack, sizePack, &pos, &temperature, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &rank, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &coord_x, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &coord_y, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &current_ip, 20, MPI_CHAR, comm_all);

        for (int j = 0; j < 4; j++)
        {
            MPI_Unpack(pack, sizePack, &pos, &adj_temperature[j], 1, MPI_INT, comm_all);
            MPI_Unpack(pack, sizePack, &pos, &adj[j], 1, MPI_INT, comm_all);
            MPI_Unpack(pack, sizePack, &pos, &adj_x[j], 1, MPI_INT, comm_all);
            MPI_Unpack(pack, sizePack, &pos, &adj_y[j], 1, MPI_INT, comm_all);
            MPI_Unpack(pack, sizePack, &pos, &adj_ip[j], 20, MPI_CHAR, comm_all);
        }

        MPI_Unpack(pack, sizePack, &pos, &t_year, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_month, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_day, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_day_name, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_hour, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_min, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_sec, 1, MPI_INT, comm_all);
        MPI_Unpack(pack, sizePack, &pos, &t_usec, 1, MPI_LONG, comm_all);

        // Calculate communication time of the message being sent from sensor node to base station
        time_com = (double)(time_receive - t_usec) / 1000000;

        // Assume the alert is false
        alert = 0; 

        // Compare the value from the message and the value from the satellite, if its true then alert is true
        if(satellite_temperature[coord_x][coord_y] >= threshold)
        {
            if (satellite_temperature[coord_x][coord_y] >= temperature - tolerance && satellite_temperature[coord_x][coord_y] <= temperature + tolerance)
            {
                alert = 1;
            }
        }

        // Output information into text file
        fprintf(fout, "----------------------------------\n");
        fprintf(fout, "Iteration : %d\n", run);
        fprintf(fout, "Logged Time :         %s %d-%d-%d %d:%d:%d\n", days[log.tm_wday], log.tm_year + 1900, log.tm_mon + 1, log.tm_mday, log.tm_hour, log.tm_min, log.tm_sec);
        fprintf(fout, "Alert Reported Time : %s %d-%d-%d %d:%d:%d\n", days[t_day_name], t_year, t_month, t_day, t_hour, t_min, t_sec);
        if(alert == 1){
            fprintf(fout, "Alert Type : True\n");
        }
        else{
            fprintf(fout, "Alert Type : False\n");
        }
        fprintf(fout, "\nReporting Node   Coord         Temp      IP\n");
        fprintf(fout, "%d                (%d,%d)         %d        %s\n", rank, coord_x, coord_y, temperature, current_ip);

        fprintf(fout, "\nAdjacent Nodes   Coord         Temp      IP\n");

        for (int j = 0; j < 4; j++)
        {
            if (adj[j] != -1)
            {
                fprintf(fout, "%d                (%d,%d)         %d        %s\n", adj[j], adj_x[j], adj_y[j], adj_temperature[j], adj_ip[j]);
            }
        }

        fprintf(fout, "\nInfrared Satellite Reporting Time (Celcius) : %s %d-%d-%d %d:%d:%d\n",days[satellite_time[coord_x][coord_y].tm_wday],satellite_time[coord_x][coord_y].tm_year + 1900, satellite_time[coord_x][coord_y].tm_mon + 1, satellite_time[coord_x][coord_y].tm_mday, satellite_time[coord_x][coord_y].tm_hour, satellite_time[coord_x][coord_y].tm_min, satellite_time[coord_x][coord_y].tm_sec);
        fprintf(fout, "Infrared Satellite Reporting (Celcius) : %d\n", satellite_temperature[coord_x][coord_y]);
        fprintf(fout, "Infrared Satellite Reporting Coord : (%d,%d)\n\n", coord_x, coord_y);

        fprintf(fout, "Communication Time (seconds) : %f\n", time_com);
        fprintf(fout, "Total Messages send between reporting node and base station: %d\n", 1);
        fprintf(fout, "Number of adjacent matches to reporting node: %d\n", match);
        fprintf(fout, "----------------------------------\n");
    }

    // Close the output
    fclose(fout);

    return 0;
}

int checkEnd(double time_start,double compare)
{
    // Time checker
    double time_check = MPI_Wtime() - time_start;

    if (time_check > compare)
    {
        return 1;
    }

    return 0;
}
