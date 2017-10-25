#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

struct timer
{
	int seconds;
	int ns;
};

struct PCB
{
	struct timer CPUTime;
	struct timer totalTime;
	struct timer prevTime;
	int priority;
	pid_t pid;
};

int errno;
char errmsg[200];
int shmidTime;
struct timer *shmTime;
int shmidPCB;
struct PCB *shmPCB;
sem_t * binSem;
/* Insert other shmid values here */


void sigIntHandler(int signum)
{
	/* Send a message to stderr */
	snprintf(errmsg, sizeof(errmsg), "OSS: Caught SIGINT! Killing all child processes.");
	perror(errmsg);	
	
	/* Deallocate shared memory */
	errno = shmdt(shmTime);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidTime, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
		perror(errmsg);	
	}

	errno = shmdt(shmPCB);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPCB)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidPCB, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPCB)");
		perror(errmsg);	
	}
	
	/* Close Semaphore */
	sem_unlink("binSem");   
    sem_close(binSem);  
	/* Exit program */
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int maxSlaves = 1;
int numSlaves = 0;
int numProc = 0;
int children[18] = {0};
int maxTime = 10;
int createNext;
char *sParam = NULL;
char *lParam = NULL;
char *tParam = NULL;
char timeArg[33];
char pcbArg[33];
char indexArg[33];
pid_t pid = getpid();
key_t keyTime = 5309;
key_t keyPCB = 8311;
FILE *fp;
char *fileName = "./msglog.out";
signal(SIGINT, sigIntHandler);
time_t start;
time_t stop;

/* Seed random number generator */
srand(pid * time(NULL));


/* Options EDIT ME LATER FOR PROJ 4 OPTIONS */
while ((o = getopt (argc, argv, "hs:l:t:")) != -1)
{
	switch (o)
	{
		case 'h':
			snprintf(errmsg, sizeof(errmsg), "oss.c simulates a primitive operating system that spawns processes and logs completion times to a file using a simulated clock.\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "oss.c options:\n\n-h\tHelp option: displays options and their usage for oss.c.\nUsage:\t ./oss -h\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-s\tSlave option: this option sets the number of slave processes from 1-19 (default 5).\nUsage:\t ./oss -s 10\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-l\tLogfile option: this option changes the name of the logfile to the chosen parameter (default msglog.out).\nUsage:\t ./oss -l output.txt\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-t\tTimeout option: this option sets the maximum run time allowed by the program in seconds before terminating (default 20).\nUsage:\t ./oss -t 5\n");
			printf(errmsg);
			exit(1);
			break;
		case 's':
			sParam = optarg;
			break;
		case 'l':
			lParam = optarg;
			break;
		case 't':
			tParam = optarg;
			break;
		case '?':
			if (optopt == 's' || optopt == 'l' || optopt == 't')
			{
				snprintf(errmsg, sizeof(errmsg), "OSS: Option -%c requires an argument.", optopt);
				perror(errmsg);
			}
			return 1;
		default:
			break;
	}	
}

/* Set maximum number of slave processes */
if(sParam != NULL)
{
	maxSlaves = atoi(sParam);
}
if(maxSlaves <= 0)
{
	maxSlaves = 1;
}
if(maxSlaves > 18)
{
	maxSlaves = 18;
}

/* Set name of log file */
if(lParam != NULL)
{
	fp = fopen(lParam, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(lParam).");
		perror(errmsg);
	}
}
else
{
	fp = fopen(fileName, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(fileName).");
		perror(errmsg);
	}
}

/* Set maximum alloted run time in real time seconds */
if(tParam != NULL)
{
	maxTime = atoi(tParam);
}

/********************MEMORY ALLOCATION********************/
/* Create shared memory segment for a struct timer */
shmidTime = shmget(keyTime, sizeof(struct timer), IPC_CREAT | 0666);
if (shmidTime < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyTime...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTime to shared memory */
shmTime = shmat(shmidTime, NULL, 0);
if ((void *)shmTime == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidTime)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a struct message */
shmidPCB = shmget(keyPCB, sizeof(struct PCB), IPC_CREAT | 0666);
if (shmidPCB < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyPCB...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTime to shared memory */
shmPCB = shmat(shmidPCB, NULL, 0);
if ((void *)shmPCB == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidPCB)");
	perror(errmsg);
    exit(1);
}
/********************END ALLOCATION********************/

/********************INITIALIZATION********************/
/* Convert shmTime and shmMsg keys into strings for EXEC parameters */
sprintf(timeArg, "%d", shmidTime);
sprintf(pcbArg, "%d", shmidPCB);


/* Set the time to 00.00 */
shmTime->seconds = 0;
shmTime->ns = 0;

/* Set the PCB array elements to 00.00, 00.00, 00.00, 0, 0 */
for(i =0; i < 18; i++)
{
	shmPCB[i].CPUTime.seconds = 0;
	shmPCB[i].CPUTime.ns = 0;
	shmPCB[i].totalTime.seconds = 0;
	shmPCB[i].totalTime.ns = 0;
	shmPCB[i].prevTime.seconds = 0;
	shmPCB[i].prevTime.ns = 0;
	shmPCB[i].priority = 0;
	shmPCB[i].pid = 0;
}
/********************END INITIALIZATION********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
binSem=sem_open("binSem", O_CREAT | O_EXCL, 0644, 1);
if(binSem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(binSem)...");
	perror(errmsg);
    exit(1);
}
/********************END SEMAPHORE CREATION********************/

/* Fork off child processes */
/*for(i = 0; i <= maxSlaves; i++)
{
	if(pid != 0 && i < maxSlaves)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS %d: About to fork!", pid);
		perror(errmsg);
		numSlaves++;
		numProc++;
		children[i] = 1;
		pid = fork();
	}
	if(pid == 0)
	{
		pid = getpid(); */
		
		/* snprintf(errmsg, sizeof(errmsg), "OSS %d: Slave process starting!", pid);
		perror(errmsg); */
		/* sprintf(pidArg, "%d", i); */
		/* execl("./user", "user", timeArg, pcbArg, (char*)0);
	}
} */


/* Start the timer */
start = time(NULL);
do
{
	for(i = 0; i < 18; i++)
	{
		if(shmPCB[i].pid == 0)
		{
			sprintf(indexArg, "%d", i);
			break;
		}
	}
	if(numSlaves < maxSlaves)
	{
		numSlaves += 1;
		numProc += 1;
		pid = fork();
		if(pid == 0)
		{
			pid = getpid();
			shmPCB[i].pid = pid;
			execl("./user", "user", timeArg, pcbArg, indexArg, (char*)0);
		}
	}
	/* if(shmMsg->pid != 0)
	{
		sem_wait(semSlaves);
		wait(shmMsg->pid);
		snprintf(errmsg, sizeof(errmsg), "OSS: Child %d is terminating at my time %02d.%d because it reached %02d.%d in slave\n", shmMsg->pid, shmTime->seconds, shmTime->ns, shmMsg->seconds, shmMsg->ns);
		fprintf(fp, errmsg);
		shmMsg->ns = 0;
		shmMsg->seconds = 0;
		shmMsg->pid = 0;
		numSlaves -= 1;
	} */
	shmTime->ns += (rand()%1000) + 1;
	if(shmTime->ns >= 1000000000)
	{
		shmTime->ns -= 1000000000;
		shmTime->seconds += 1;
	}
	shmTime->seconds += 1;
	stop = time(NULL);
/* }while(stop-start < maxTime && shmTime->seconds < 2 && numProc < 100 + maxSlaves); */
}while(stop-start < maxTime && numProc < 100);

for(i = 0; i < 18; i++)
{
	printf("children[%d] = %d\n", i, shmPCB[i].pid);
}

/* if(shmTime->seconds >= 2)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: 2 simulated seconds have passed.");
	perror(errmsg);
} */


/* Kill all slave processes */
/* for(i = 1; i <= numProc; i++)
{
	kill(pid[i], SIGINT);
} */
/********************DEALLOCATE MEMORY********************/
errno = shmdt(shmTime);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
	perror(errmsg);	
}

errno = shmctl(shmidTime, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
	perror(errmsg);	
}

errno = shmdt(shmPCB);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPCB)");
	perror(errmsg);	
}

errno = shmctl(shmidPCB, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPCB)");
	perror(errmsg);	
}
/********************END DEALLOCATION********************/

/* Close Semaphore */
sem_unlink("binSem");   
sem_close(binSem);

printf("OSS: Program completed successfully!\n");
return 0;
}
