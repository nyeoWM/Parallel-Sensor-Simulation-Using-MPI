##############################################################
# Make file for Distributed Wireless Sensor Network simulation
#   instructions in README
##############################################################
all: build

build: main.c basestation.c sensor.c
# Checks if outputStation.txt file exists, else creates file
ifeq (,$(wildcard "outputStation.txt" ))
	@true
else 
	echo " " >>outputStation.txt endif
endif 
# Checks if inputStation.txt file exists, else creates file
ifeq (,$(wildcard "inputStation.txt"))
	@true
else 
	echo " " >>inputStation.txt endif
endif
	# Compiles program
	mpicc -fopenmp main.c basestation.c sensor.c -o DWSN.o -lm

run:
	# prompts the user for the neccesary inputs then runs the program 
	@read -p "Enter number of processors:" p; \
	read -p "Enter length of matrix:" n; \
	read -p "Enter height of matrix:" m;\
	read -p "Enter number of iterations, -1 for infinite iterations:" i; \
	mpirun -np $$p ./DWSN.o $$n $$m $$i

clean:
	# removes all output files and creates an empty inputStation file
	rm -f *.o
	rm -f inputStation.txt
	echo " " >>inputStation.txt

