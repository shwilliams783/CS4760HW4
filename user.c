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
pid_t pid;
char errmsg[100];
struct timer *shmTime;
struct message *shmMsg;
sem_t * sem;
sem_t * semSlaves;
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

	errno = shmdt(shmMsg);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmMsg)", pid);
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
int msgKey = atoi(argv[2]);
key_t keyTime = 8675;
key_t keyMsg = 1138;
signal(SIGINT, sigIntHandler);
pid = getpid();

/* Seed random number generator */
srand(pid * time(NULL));

/* snprintf(errmsg, sizeof(errmsg), "USER %d: Slave process started!", pid);
perror(errmsg); */

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
shmMsg = shmat(msgKey, NULL, 0);
if ((void *)shmMsg == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidMsg)");
	perror(errmsg);
    exit(1);
}
/********************END ATTACHMENT********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
sem=sem_open("pSem", 1);
if(sem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(pSem)...", pid);
	perror(errmsg);
    exit(1);
}

semSlaves = sem_open("slaveSem", 1);
if(semSlaves == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(slaveSem)...", pid);
	perror(errmsg);
    exit(1);
}
/********************END SEMAPHORE CREATION********************/

/* Calculate End Time */
endNS = shmTime->ns + (rand()%10000) + 1;
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
/* printf("USER %d: endNS = %d endSec = %d shmTime->ns = %d shmTime->seconds = %d\n", pid, endNS, endSec, shmTime->ns, shmTime->seconds); */

 /* Busy-Wait until oss.c clears the message */
while(shmMsg->pid != 0);

/********************ENTER CRITICAL SECTION********************/
sem_wait(sem);	/* P operation */
shmMsg->ns = endNS;
shmMsg->seconds = endSec;
shmMsg->pid = pid;
sem_post(sem); /* V operation */  
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

	errno = shmdt(shmMsg);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "MASTER: shmdt(shmMsg)");
		perror(errmsg);	
	}
/********************END DETACHMENT********************/

/* Post to semSlaves */
sem_post(semSlaves);
/* printf("USER: Child Finished!\n"); */
exit(0);
return 0;
}
