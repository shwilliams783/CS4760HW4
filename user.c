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
pid_t pid;
char errmsg[100];
struct timer *shmTime;
struct PCB *shmPCB;
sem_t * binSem;
/* Insert other shmid values here */

void sigIntHandler(int signum)
{
	snprintf(errmsg, sizeof(errmsg), "USER %d: Caught SIGINT! Killing all child processes.", pid);
	perror(errmsg);	
	errno = shmdt(shmTime);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmTime)", pid);
		perror(errmsg);	
	}
	errno = shmdt(shmPCB);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmPCB)", pid);
		perror(errmsg);	
	}
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int endSec;
int endNS;
int timeKey = atoi(argv[1]);
int pcbKey = atoi(argv[2]);
int index = atoi(argv[3]);
signal(SIGINT, sigIntHandler);
pid = getpid();

/* Seed random number generator */
srand(pid * time(NULL));

snprintf(errmsg, sizeof(errmsg), "USER %d: Slave process started!", pid);
perror(errmsg);

/********************MEMORY ATTACHMENT********************/
/* Point shmTime to shared memory */
shmTime = shmat(timeKey, NULL, 0);
if ((void *)shmTime == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidTime)");
	perror(errmsg);
    exit(1);
}

/* Point shmTime to shared memory */
shmPCB = shmat(pcbKey, NULL, 0);
if ((void *)shmPCB == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidPCB)");
	perror(errmsg);
    exit(1);
}
/********************END ATTACHMENT********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
binSem=sem_open("binSem", 1);
if(binSem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(binSem)...", pid);
	perror(errmsg);
    exit(1);
}

/********************END SEMAPHORE CREATION********************/

snprintf(errmsg, sizeof(errmsg), "USER %d: Shared memory working!", shmPCB[index].pid);
perror(errmsg);


/* Calculate End Time */
endNS = shmTime->ns + (rand()%1000) + 1;
endSec = shmTime->seconds;
if (endNS > 1000000000)
{
	endNS -= 1000000000;
	endSec += 1;
}

/* printf("USER %d: endNS = %d endSec = %d\nshmTime->ns = %d shmTime->seconds = %d\n", pid, endNS, endSec, shmTime->ns, shmTime->seconds); */

/* Wait for the system clock to pass the time */
while(endSec != shmTime->seconds);
while(endNS > shmTime->ns);
printf("USER %d: endNS = %d endSec = %d shmTime->ns = %d shmTime->seconds = %d\n", pid, endNS, endSec, shmTime->ns, shmTime->seconds);


/********************ENTER CRITICAL SECTION********************/
/*sem_wait(sem); */	/* P operation */
/* shmMsg->ns = endNS;
shmMsg->seconds = endSec;
shmMsg->pid = pid; */
/* sem_post(sem); */ /* V operation */  
/********************EXIT CRITICAL SECTION********************/

/* snprintf(errmsg, sizeof(errmsg), "USER %d: shmTime->seconds = %d shmTime->ns = %d\n", pid, shmTime->seconds, shmTime->ns);
perror(errmsg);
snprintf(errmsg, sizeof(errmsg), "USER %d: shmMsg->pid = %d shmMsg->seconds = %d shmMsg->ns = %d\n", pid, shmMsg->pid, shmMsg->seconds, shmMsg->ns);
perror(errmsg); */

/********************MEMORY DETACHMENT********************/
errno = shmdt(shmTime);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "MASTER: shmdt(shmTime)");
	perror(errmsg);	
}

errno = shmdt(shmPCB);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmPCB)", pid);
	perror(errmsg);	
}
/********************END DETACHMENT********************/
printf("USER: Child Finished!\n");
exit(0);
return 0;
}
