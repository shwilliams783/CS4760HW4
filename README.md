# CS4760HW4
Operating System Simulator Part 2

# Original Specification Document:

# Process Scheduling

In this project, you will simulate the process scheduling part of an operating system. You will implement time-based scheduling,
ignoring almost every other aspect of the OS.

# Operating System Simulator

This will be your main program and serve as the master process. You will start the operating system simulator (call the executable
oss) as one main process who will fork multiple children at random times. The randomness will be simulated by a logical clock that
will also be updated by oss.

In the beginning, oss will allocate shared memory for system data structures, including process control block for each user process.
The process control block is a fixed size structure and contains information on managing the child process scheduling. Notice that
since it is a simulator, you will not need to allocate space to save the context of child processes. But you must allocate space for
scheduling-related items such as total CPU time used, total time in the system, time used during the last burst, and process priority, if
any. The process control block resides in shared memory and is accessible to the child. Since we are limiting ourselves to 20 processes
in this class, you should allocate space for up to 18 process control blocks. Also create a bit vector, local to oss, that will help you
keep track of the process control blocks (or process IDs) that are already taken.
oss will create user processes at random intervals, say every second on an average. The clock itself will get incremented in terms of
nanoseconds. I’ll suggest that you have two unsigned integers for the clock; one will show the time in seconds and the other will show
the time in nanoseconds, offset from the beginning of a second. The clock will be accessible to every process and hence, in shared
memory. It will be advanced only by oss though it can be observed by all the children to see the current time.
oss will run concurrently with all the user processes. After it sets up all the data structures, it enters a loop where it generates and
schedules processes. It generates a new process by allocating and initializing the process control block for the process and then, forks
the process. The child process will execl the binary.
Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds. xx will be a random
number in the interval [0,1000] to simulate some overhead activity for each iteration.
A new process should be generated every 1 second, on an average. So, you should generate a random number between 0 and 2
assigning it to time to create new process. If your clock has passed this time since the creation of last process, generate a new process
(and execl it). If the process table is already full, do not generate any more processes.
Scheduling Algorithm. oss will select the process to be run and schedule it for execution. It will select the process by using a
scheduling algorithm with the following features:
Implement a version of multi-level scheduling. There are three priority queues – a high-priority queue (queue 0), a medium priority
queue (queue 1), and a low-priority queue (queue 2).
Initially, all processes enter queue 0. A process is moved to queue 1 whenever its waiting time is higher than some threshhold (to be
decided by you) and is also higher than
α × average waiting time of processes in queue #1
A process is moved from queue 1 to queue 2 whenever its waiting time is higher than some threshold (to be decided by you) and is
also higher than
β × average waiting time of processes in queue #2
Here, α and β are two constants chosen by you. You can change them to tune performance. I’ll prefer if you #define them in the
beginning of code. You should play with the values of the threshold, α, β, the time slices for queues, and the time quantum to obtain
good throughput.
Every time a process is scheduled, you will generate a random number in the range [0, 3] where 0 indicates that the process terminates,
1 indicates that the process terminates at its time quantum, 2 indicates that process starts to wait for an event that will last for r.s
seconds where r and s are random numbers with range [0, 5] and [0, 1000] respectively, and 3 indicates that the process gets preempted
after using p of its assigned quantum, where p is a random number in the range [1, 99].
oss will start by picking processes from queue 0, then 1, and finally 2. It will assign a quantum q to processes in queue 0, q/2 to
processes in queue 1, and q/4 to processes in queue 2. Once it has gone through all the processes, it will start with processes in queue
0 again.
The process will be dispatched by putting the process ID and the time quantum in a predefined location in shared memory. The user
process will pick up the quantum value from the shared memory and schedule itself to run for that long a time.

# User Processes

All user processes are alike but simulate the system by performing some tasks at random times. The user process will keep checking
in the shared memory location if it has been scheduled and once done, it will start to run. It should generate a random number to check
whether it will use the entire quantum, or only a part of it (a binary random number will be sufficient for this purpose). If it has to
use only a part of the quantum, it will generate a random number in the range [0,quantum] to see how long it runs. After its allocated
time (completed or partial), it updates its process control block by adding to the accumulated CPU time. It joins the ready queue at that
point and sends a signal on a semaphore so that oss can schedule another process.
While running, generate a random number again to see if the process is completed. This should be done if the process has accumulated
at least 50 milliseconds. If it is done, the message should be conveyed to oss who should remove its process control block.
Your simulation should end with a report on average turnaround time and average wait time for the processes. Also include how long
the CPU was idle.
Make sure that you have signal handing to terminate all processes, if needed. In case of abnormal termination, make sure to remove
shared memory and semaphores, if you used them.

I HIGHLY SUGGEST YOU DO THIS PROJECT INCREMENTALLY. Have master create the PCB and work with just one child user
process and get the clock working. Once this is working, then start working on maybe a one queue scheduler doing round-robin. Then
start adding to the system. Do not try to do everything at once and be stuck with no idea what is failing.

# Log Output

Your program should send enough output to a log file such that it is possible for me to determine its operation. For example:

OSS: Generating process with PID 3 and putting it in queue 1 at time 0:5000015
OSS: Dispatching process with PID 2 from queue 1 at time 0:5000805,
OSS: total time this dispatch was 790 nanoseconds
OSS: Receiving that process with PID 2 ran for 400000 nanoseconds
OSS: Putting process with PID 2 into queue 2
OSS: Dispatching process with PID 3 from queue 1 at time 0:5401805,
OSS: total time this dispatch was 1000 nanoseconds
OSS: Receiving that process with PID 3 ran for 270000 nanoseconds,
OSS: not using its entire time quantum
OSS: Putting process with PID 3 into queue 1
OSS: Dispatching process with PID 1 from queue 1 at time 0:5402505,
OSS: total time spent in dispatch was 7000 nanoseconds
etc

I suggest not simply appending to previous logs, but start a new file each time. Also be careful about infinite loops that could generate
excessively long log files. So for example, keep track of total lines in the log file and terminate writing to the log if it exceeds 10000
lines.
Note that the above log was using arbitrary numbers, so your times spent in dispatch could be quite different.

# Deliverables

Handin an electronic copy of all the sources, README, Makefile(s), and results. Create your programs in a directory called username.4
where username is your login name on hoare. Once you are done with everything, remove the executables and object files, and issue
the following commands:

chmod 700 username.4

cp -r username.4 /home/hauschild/cs4760/assignment4

Do not forget Makefile (with suffix rules), version control, and README for the assignment. If you do not use version control, you
will lose 10 points. Omission of a Makefile (with suffix rules) will result in a loss of another 10 points, while README will cost
you 5 points.
