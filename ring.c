#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define PING 1 
#define PONG -1 
#define LOSS -2
#define null -999999

int * graphConstruct(int size, int *sites);
int * showGraph(int size, int *sites);
int popProcess(int *list, int *size, int toPop);
void regenerate(int x, int *ping, int *pong);
void incarnate( int x, int *ping, int *pong);
int simulateLoss(int prob);


int main(int argc, char *argv[])
{ 
	// Variable Declarations
	int rank, size, tag = 201;

	//Declaration of the graph
  	int *sites,i;

  	//Connected sites
  	int *connected, *connectAssig;

	MPI_Status  status;

	//PING & PONG for Misra Algorithm
	int ping[2]={PING,1}, pong[2]={PONG,-1}, last = null, msg[2]={0,0};

	//Declaration of variables to count
	//We implement a sort of timer for the token
	int count=0, niter=atoi(argv[1]);

	//Probability to have a loss
	int prob = atoi(argv[2]);

	//Activate when a token is lost
	int loose = LOSS;

	// Start MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	srand(time(NULL));
	//Declaration of the previous and the next site
	//connected[0]: Previous site
	//connected[1]: Next site
	connected = (int*)malloc(2*sizeof(int));
	connectAssig = (int*)malloc(2*sizeof(int));

	if( rank == 0 ) {
		sites = graphConstruct(size,sites);
		sites = showGraph(size,sites);
		for(i=0;i<size;i++) {
			if(sites[i] !=0){
				connectAssig[0] = sites[(i-1+size)%size]; //Complexe operation to have the loop assignation
				connectAssig[1] = sites[(i+1+size)%size];
      			MPI_Send(connectAssig, 2, MPI_INT, sites[i], tag, MPI_COMM_WORLD);
			} else {
				connected[0] = sites[(i-1+size)%size];
				connected[1] = sites[(i+1+size)%size];
			}
		}
		MPI_Send(&ping, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
		printf("%d PING %d\n", rank,connected[1]);
		//usleep(50);
		MPI_Send(&pong, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
		printf("%d PONG %d\n", rank,connected[1]);
	} else {
		//Neighboor for the first time
    	MPI_Recv(connected, 2, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	while(pong[1] > (-1)*niter){
		MPI_Recv(&msg, 2, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(msg[0] == PING){
			if(last == msg[1]){
				regenerate(msg[1],ping,pong);
				loose = PONG;
				printf("%d:PONG REGENERATED\n",rank );
			} 
/*			else {
				incarnate(msg[1],ping,pong);
			}*/
			MPI_Send(&msg, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
			last = msg[1];
			printf("%d PING %d\n", rank,connected[1]);
		} else if(msg[0] == PONG){
			if(last == msg[1]){
				regenerate(msg[1],ping,pong);
				loose = PING;
			} else {
				incarnate(msg[1],ping,pong);
			}
			if(simulateLoss(prob) == 1){
				int loss[2]={LOSS,-1};
				MPI_Send(&loss, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
			} else {
				MPI_Send(&pong, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
			}
			last = pong[1];
			printf("%d PONG %d\n", rank,connected[1]);
		} else {
			printf("%d:PONG LOST\n",rank );
		}

		if(loose != LOSS){
			if(loose == PONG) {
				MPI_Send(&pong, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
				last = pong[1];
				printf("%d PONG1 %d\n", rank,connected[1]);
			} else {
				MPI_Send(&ping, 2, MPI_INT, connected[1], tag, MPI_COMM_WORLD);
				last = ping[1];
				printf("%d PING2 %d\n", rank,connected[1]);
			}
			loose = LOSS;
		}
	}
	free(sites);
	MPI_Finalize();
	return 0;
}


int * graphConstruct(int size, int *sites){
	srand(time(NULL));
	int i, toPop;
	int *ringSites, sizeRing=size;
	//To verify if a process is at least attached to another one
	int temp = 0 ;
	srand(time(NULL));
	sites = (int*)malloc(size*sizeof(int));
	ringSites = (int*)malloc(size*sizeof(int));
	for(i=0;i<size;i++){
		ringSites[i] = i;
	}
	while(sizeRing > 0){
		temp=rand()%sizeRing;
		toPop = popProcess(ringSites, &sizeRing, ringSites[temp]);
		sites[size-sizeRing-1] = toPop;
	}
	return sites;
}


int * showGraph(int size, int *sites) {
	int i;
	printf("PROCESS ASSIGNATION:\n");
	for(i=0;i<size;i++){
	  printf("\t%d",sites[i]);
	}
	printf("\n");
  return sites;
}

int popProcess(int *list, int *size, int toPop){
  int i;
  int popped=-1;
  for(i=0;i<*size;i++){
    if(list[i] == toPop){
      popped=list[i];
    }
    if(popped != -1 && i!= *size-1){
      list[i] = list [i+1];
    }
  }
  *size = *size - 1;
  if(*size == 0){
    free(list);
  } else {
    list = realloc (list, *size * sizeof(int) );
  }
  return popped;
}

void regenerate(int x, int *ping, int *pong) {
	ping[1] = abs(x);
	pong[1] = (-1)*(ping[1]);
}

void incarnate( int x, int *ping, int *pong) {
	ping[1] = abs(x) + 1;
	pong[1] = (-1)*(ping[1]);
}

int simulateLoss(int prob){
	srand(time(NULL));
	int temp = rand()%prob;
	if(temp == 0){
		return 1;
	} else {
		return -1;
	}
}