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

struct message
{
	pid_t pid;
	int seconds;
	int ns;
};

int errno;
char errmsg[200];
int shmidTime;
int shmidMsg;
struct timer *shmTime;
struct message *shmMsg;
sem_t * sem;
sem_t * semSlaves;
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

	errno = shmdt(shmMsg);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmMsg)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidMsg, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidMsg)");
		perror(errmsg);	
	}
	
	/* Close Semaphore */
	sem_unlink("pSem");   
    sem_close(sem);  
	sem_unlink("slaveSem");
	sem_close(semSlaves);
	/* Exit program */
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int maxSlaves = 5;
int numSlaves = 0;
int numProc = 0;
int maxTime = 20;
char *sParam = NULL;
char *lParam = NULL;
char *tParam = NULL;
char timeArg[33];
char msgArg[33];
pid_t pid[101] = {getpid()};
pid_t myPid;
key_t keyTime = 8675;
key_t keyMsg = 1138;
FILE *fp;
char *fileName = "./msglog.out";
signal(SIGINT, sigIntHandler);
time_t start;
time_t stop;



/* Options */
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
if(maxSlaves < 0)
{
	maxSlaves = 5;
}
if(maxSlaves > 19)
{
	maxSlaves = 19;
}

/* Set name of log file */
if(lParam != NULL)
{
	fp = fopen(lParam, "a");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(lParam).");
		perror(errmsg);
	}
}
else
{
	fp = fopen(fileName, "a");
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
shmidMsg = shmget(keyMsg, sizeof(struct message), IPC_CREAT | 0666);
if (shmidMsg < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyMsg...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTime to shared memory */
shmMsg = shmat(shmidMsg, NULL, 0);
if ((void *)shmMsg == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidMsg)");
	perror(errmsg);
    exit(1);
}
/********************END ALLOCATION********************/

/********************INITIALIZATION********************/
/* Convert shmTime and shmMsg keys into strings for EXEC parameters */
sprintf(timeArg, "%d", shmidTime);
sprintf(msgArg, "%d", shmidMsg);


/* Set the time to 00.00 */
shmTime->seconds = 0;
shmTime->ns = 0;
/* printf("shmTime->seconds = %d shmTime->ns = %d\n", shmTime->seconds, shmTime->ns); */

/* Set the message to 0, 00.00 */
shmMsg->pid = 0;
shmMsg->seconds = 0;
shmMsg->ns = 0;
/* printf("shmMsg->pid = %d shmMsg->seconds = %d shmMsg->ns = %d\n", shmMsg->pid, shmMsg->seconds, shmMsg->ns); */
/********************END INITIALIZATION********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
sem=sem_open("pSem", O_CREAT | O_EXCL, 0644, 1);
if(sem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(pSem)...");
	perror(errmsg);
    exit(1);
}
semSlaves=sem_open("slaveSem", O_CREAT | O_EXCL, 0644, maxSlaves);
if(semSlaves == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(slaveSem)...");
	perror(errmsg);
    exit(1);
}
/********************END SEMAPHORE CREATION********************/

/* Fork off child processes */
for(i = 0; i <= maxSlaves; i++)
{
	if(pid[i] != 0 && i < maxSlaves)
	{
		pid[i+1] = fork();
		numSlaves++;
		numProc++;
		continue;
	}
	else if(pid[i] == 0)
	{
		execl("./user", "user", timeArg, msgArg, (char*)0);
	}
}

/* Start the timer */
start = time(NULL);
do
{
	if(pid[numProc] != 0)
	{
		if(numSlaves < maxSlaves)
		{
			numSlaves += 1;
			numProc += 1;
			pid[numProc] = fork();
			if(pid[numProc] == 0)
			{
				execl("./user", "user", timeArg, msgArg, (char*)0);
			}
		}
		if(shmMsg->pid != 0)
		{
			sem_wait(semSlaves);
			wait(shmMsg->pid);
			snprintf(errmsg, sizeof(errmsg), "OSS: Child %d is terminating at my time %02d.%d because it reached %02d.%d in slave\n", shmMsg->pid, shmTime->seconds, shmTime->ns, shmMsg->seconds, shmMsg->ns);
			fprintf(fp, errmsg);
			shmMsg->ns = 0;
			shmMsg->seconds = 0;
			shmMsg->pid = 0;
			numSlaves -= 1;
		}
		shmTime->ns += (rand()%10000) + 1;
		if(shmTime->ns >= 1000000000)
		{
			shmTime->ns -= 1000000000;
			shmTime->seconds += 1;
		}
		/*pid = getpid();
		snprintf(errmsg, sizeof(errmsg), "OSS: SS = %02d NS = %d, PID = %d\n", shmTime->seconds, shmTime->ns, pid);
		printf(errmsg); */
		stop = time(NULL);
	}
	/* myPid = getpid();
	snprintf(errmsg, sizeof(errmsg), "OSS: myPid = %d numProc = %d, numSlaves = %d, maxSlaves = %d\n", myPid, numProc, numSlaves, maxSlaves);
	printf(errmsg); */
}while(stop-start < maxTime && shmTime->seconds < 2 && numProc < 100 + maxSlaves);
/* }while(stop-start < maxTime && numProc < 100); */

if(shmTime->seconds >= 2)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: 2 simulated seconds have passed.");
	perror(errmsg);
}


/* Kill all slave processes */
for(i = 1; i <= numProc; i++)
{
	/* printf("Killing process #%d\n", pid[i]); */
	kill(pid[i], SIGINT);
}
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

errno = shmdt(shmMsg);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmMsg)");
	perror(errmsg);	
}

errno = shmctl(shmidMsg, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidMsg)");
	perror(errmsg);	
}
/********************END DEALLOCATION********************/

/* Close Semaphore */
sem_unlink("pSem");   
sem_close(sem);
sem_unlink("slaveSem");
sem_close(semSlaves);

return 0;
}
