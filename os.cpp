#include "os.h"

//Macros
#define job_number		1
#define job_priority	2
#define job_size		3
#define time_remaining	4
#define time_arrival	5
#define core_addr		6

//Debug defines
#define crint	7
#define dskint	8
#define drmint	9
#define tro 	10
#define svc		11

//Global Data Structures
MemoryManager memManager; // Representation of FST
list<PCB> cpuReadyQueue; // List of jobs that are capable of running on CPU & other operations
list<PCB> ioQueue; // Queue of jobs asking for IO (Each job can have more than one instance)
list<PCB>::iterator runningJob; // Pointer to job that is currently running
queue<PCB> shortTermSch; // Jobs that found space in memory, but have not been swapped by the drum yet
multimap<long, PCB, less<long> > longTermSch; // LTS that sorts jobs by maxCPUTime when they do not find space in memory

//Global Variables
long TIME_SLICE; // Time quantum given to a job when it's dispatched to CPU

bool drumBusy; // Semaphore to check if drum is in use (Swapping a job in core)
bool diskBusy; // Semaphore to check if disk is in use (A job is currently doing IO)

//Prototypes
void siodisk(long jobNum);
void siodrum(long jobNum, long jobSize, long coreAddr, long direction);
void ontrace();
void offtrace();
void dispatcher(long &, long *);
void longTermScheduler(); // Loads a job into memory. Jobs located here did not find space in memory when they were Crinted in
void bookKeeping(long &a, long p[]); // Checks the job status. Terminates, unblocks, etc.
long assignCorrectTimeQuantum(); // Assigns time quantum (Either TIME_SLICE or remaining CPU time)
void moveJobPtr(); // Moves job pointer forward in cpuReadyQueue
void testFunc(int); // Debugging function


/************************************************************************************
**			This program is an operating system simulator                          **
**			We use a best free space table to store the free                       **
**			spaces. We use a round robin cpu scheduling algorithm.                 **
**			Our cpu scheduling algorithm is non preemptive, which means            **
**			at every interrupt besides svc (where a equals to 5 or 7) we           **
**			and Tro we give the cpu back to the job that was interrupted           **
**  																		       **
**			By Rakib Hasan, Jeremy Levine, Michael Figueroa & Frank Gassoso        **
************************************************************************************/

/*
**     This function initializes values
*/
void startup()
{
    TIME_SLICE = 400;
    drumBusy = false;
    diskBusy = false;
    offtrace();
}

/*
**                       Crint(2)
**           by Frank Gassoso and Jeremy Levine
**
**    This function gets called when the job spooler
**    sends a new job to the system. The function looks
**    for a space inside the memory table. If it finds
**    space then the program will be put onto the
**    short term scheduler. Excess space is put back
**    into the memory table. Then the defragment
**    function is called. If memory is not found then
**    the program is stored in long term scheduler.
*/
void Crint(long &a, long p[])
{
    PCB newJob(p[job_number],p[job_priority],p[job_size],p[time_remaining],p[time_arrival]);
    pair<long, long> freeSpace;
	
    // Looks for free-space in memory
    freeSpace = memManager.findSpace(newJob[job_size]);
	
    if(freeSpace.first != 0) {
		// If found push onto short term scheduler
		newJob.setJobAddr(freeSpace.second);
		shortTermSch.push(newJob);
		// Insert excess space
		memManager.insertInTable(pair<long,long>(freeSpace.first - newJob[job_size],
		freeSpace.second + newJob[job_size]));
		memManager.defragment();
    }
	else
		// If not put onto LTS
        longTermSch.insert(pair<long, PCB>(newJob[time_remaining], newJob));
		
    if(!shortTermSch.empty()&&!drumBusy) {   
		// Call siodrum for jobs on shorttermsch
        PCB temp(shortTermSch.front());
        siodrum(temp[job_number], temp[job_size], temp[core_addr], 0);
        drumBusy = true;
    }
	
    if(!ioQueue.empty()&&!diskBusy) {   
		// Call siodisk for jobs on IOQueue
        siodisk(ioQueue.front()[job_number]);
        diskBusy = true;
    }
	
    if(!cpuReadyQueue.empty()) {
        bookKeeping(a,p);
        dispatcher(a,p);
    } 
	else
		a = 1;
    //testFunc(crint);
}

/*
**                         Drmint(2)
**            by Frank Gassoso and Jeremy Levine
**
**    This function gets called when after siodrum is called
**    for a program. It takes the front value of the short
**    term scheduler and pushing it onto cpuReadyQueue. If
**    the cpuReadyQueue was empty before puting a new job
**    on the cpuReadyQueue, then it sets the running job pointer
**    rqIt to the beginning of the cpuReadyQueue
*/
void Drmint(long &a, long p[])
{
	bool startRRFromBeginning = cpuReadyQueue.empty();
    cpuReadyQueue.push_back(shortTermSch.front());
    shortTermSch.pop();
	
	// If cpuReadyQueue was empty before Drmint(2) called
    //then set runningJob to beginning of readyQueue
    if(startRRFromBeginning)
		runningJob = cpuReadyQueue.begin();
		
	// If IOQueue not empty do IO for job in front of list
    if(!ioQueue.empty() && !diskBusy) {
          siodisk(ioQueue.front()[job_number]);
          diskBusy = true;
    }
	
    if(!shortTermSch.empty() && !drumBusy) {   
		// Call siodrum for jobs on STS
        PCB temp(shortTermSch.front());
        siodrum(temp[job_number], temp[job_size], temp[core_addr], 0);
        drumBusy = true;
    }
	
    if(!cpuReadyQueue.empty()) {
        bookKeeping(a,p);
        dispatcher(a,p);
    }
	else
		a = 1;
		
    drumBusy = false;
    //testFunc(drmint);
}

/*
**                       Dskint(2)
**            by Frank Gassoso, Jeremy Levine
**
**    This function gets called after siodisk(1) gets
**    called for a  job. It decrements the ioCnt for the job
**    that called siodisk(1)
*/
void Dskint(long &a, long p[])
{
    list<PCB>::iterator it;
	
    for(it = cpuReadyQueue.begin(); it != cpuReadyQueue.end(); it++) {
		if(*it == ioQueue.front()) {
            it->decIOcnt();
			break;
        }
    }
	// Pop IO Queue (Front is job that just finished doing IO)
    ioQueue.pop_front();
	
    if(!cpuReadyQueue.empty()) {
        bookKeeping(a,p);
        dispatcher(a,p);
    }
	else
		a = 1;
		
    diskBusy = false; // Set diskBusy to false. Dskint is called when the disk finishes its current IO
	
    if(!ioQueue.empty() && !diskBusy) {   
		// Call siodisk for jobs on ioqueue
        siodisk(ioQueue.front()[job_number]);
        diskBusy = true;
    }
	
    //testFunc(dskint);
}

/*
**                             Tro(2)
**        by Frank Gassoso, Jeremy Levine, Rakib Hasan
**        and Michael Figueroa
**
**    This function gets called when the time quantum for a
**    job is completed. We set the jobRunning bit for the function
**    to false, and decrement time that it has been execueting since
**    it was dispatched from its the program's time remaining.
**    If there is no more time remaining for the job then the program
**    has terminated abnormally and we have to set the terminated bit
**    for the job to true.
*/
void Tro(long &a, long p[5])
{
    runningJob->setJobRunning(false);
    runningJob->decrementTimeRemaining(p[5]-runningJob->getStartingTimeExecution());
	
    if(runningJob->getTimeRemaining() == 0)   
		// If job has no time remaining on cpu then terminate
        runningJob->setTerminated(true);
		
    moveJobPtr(); // Move job pointer to next job in cpuReadyQueue
	
    if(!shortTermSch.empty() && !drumBusy) {
        // Call siodrum for jobs on STS
        PCB temp(shortTermSch.front());
        siodrum(temp[job_number], temp[job_size], temp[core_addr], 0);
        drumBusy = true;
    }
	
    if(!ioQueue.empty() && !diskBusy) {   
		// Call siodisk for jobs on ioqueue
        siodisk(ioQueue.front()[job_number]);
        diskBusy = true;
    }

    if(!cpuReadyQueue.empty()) {
        bookKeeping(a,p);
        dispatcher(a,p);
    }
	else
        a = 1;
		
    //testFunc(tro);
}

/*
**                            Svc(2)
**          by Frank Gassoso, Jeremy Levine, Rakib Hasan
**          and Michael Figueroa
**
**    This function gets called when the running job wants
**    either, terminate, do IO, or be blocked until all it's
**    IO is completed. If it wants to terminate then we set
**    the terminate bit to true, if it wants to be blocked
**    we check if it has pendingIO and if it does then we
**    block it. If it wants to do IO then we increment it's
**    IO count and put it on the ioQueue.
*/
void Svc(long &a, long p[])
{
    switch(a) {
        case 5:
            // Terminate job
            runningJob->setTerminated(true);
            runningJob->setJobRunning(false);
            runningJob->decrementTimeRemaining(p[5]-runningJob->getStartingTimeExecution());
            moveJobPtr();
			break;
        case 6:
            // Put job on ioQueue
            runningJob->incIOcnt();
            ioQueue.push_back(*runningJob);
			break;
        case 7:
            // Block jobs if it has at least one pending IO
            if(runningJob->isPendingIO()) {
                runningJob->setBlocked(true);
              //  runningJob->setStartingTimeBlocked(p[5]);
                runningJob->setJobRunning(false);
                runningJob->decrementTimeRemaining(p[5]-runningJob->getStartingTimeExecution());
                moveJobPtr();
            }
			break;
    }
	
    if(!shortTermSch.empty() && !drumBusy) {
        // Call siodrum for jobs on shorttermsch
        PCB temp(shortTermSch.front());
        siodrum(temp[job_number], temp[job_size], temp[core_addr], 0);
        drumBusy = true;
    }
	
    if(!ioQueue.empty() && !diskBusy) {   
		// Call siodisk for jobs on ioqueue
        siodisk(ioQueue.front()[job_number]);
        diskBusy = true;
    }

    if(!cpuReadyQueue.empty()) {
        bookKeeping(a,p);
        dispatcher(a,p);
    }
	else
		a = 1;
		
    //testFunc(svc);
}

/*
**                        dispatcher(2)
**
**    This function gets called at the end of every interrupt
**    only when the cpuReadyQueue is not empty. It checks if
**    there are any jobs in the cpuReadyQueue where the blocked
**    bit and the terminate bit are not set. It gives that job the
**    cpu. It gives the jobs to cpu in First come first serve order.
**    If all the jobs in the cpuReadyQueue have either their blocked
**    bit or terminate bit set to true then the cpu will be set to idle
**
**              By Rakib Hasan and Michael Figueroa
*/
void dispatcher(long &a, long p[])
{
    // Put CPU in running mode
    int rqLen = cpuReadyQueue.size();
    while(rqLen > 0) {
		if(runningJob->isBlocked() || runningJob->isTerminated()) {
			moveJobPtr();
			rqLen--;
		}
		else
			break;
    }
	
    if(rqLen == 0) {
         a = 1;
	}
    else {
        a = 2;
        // Store correct values in register
        p[2] = runningJob->getJobAddress();
        p[3] = runningJob->getJobSize();
        p[4] = assignCorrectTimeQuantum();
		
        // Store starting time of process somewhere
        runningJob->setStartingTimeExecution(p[5]);
        runningJob->setJobRunning(true);
    }
}

/*
**                      bookKeeping(2)
**             by Rakib Hasan and Michael Figueroa
**
**    This function gets called at the end of every interrupt
**    if the cpuReadyQueue is not empty. For each member of the
**    cpuReadyQueue, if there is pending IO then it sets the
**    pendingIO bit to true, if the terminate bit is set and
**    pendingIO bit is set to false then it stores the job space
**    into the free space table, if it's  blocked bit is set to
**    true and the pendingIO bit is false then it unblocks the job,
**    if the jobRunning bit is set to true then it decrements the
**    the time since it started execution from it's time remaining
*/
void bookKeeping(long &a, long p[])
{
    queue<list<PCB>::iterator > eraseQ;
    list<PCB>::iterator it;
	
    for(it = cpuReadyQueue.begin(); it != cpuReadyQueue.end(); it++) {   //set pending io to true if io count is greater then 0
        if(it->getIOcnt() > 0)
			it->setPendingIO(true);
        else
            it->setPendingIO(false);
			
		// If a job is terminated and has no pending IO then add mem to free space remove from RQ
        if(it->isTerminated() && !it->isPendingIO()) {
            memManager.insertInTable(pair<long,long>((*it)[job_size], (*it)[core_addr]));
            memManager.defragment();
            eraseQ.push(it);
			
            if(*runningJob == *it)
                moveJobPtr();
				
            if(!longTermSch.empty())
				longTermScheduler();
        }
		
		// If a job is blocked and not pending io then unblock the job
        if(it->isBlocked() && !it->isPendingIO())
            it->setBlocked(false);
      
        if(it->isRunning())
			it->decrementTimeRemaining(p[5]-it->getStartingTimeExecution());
    }
	
    while(!eraseQ.empty()) {
        cpuReadyQueue.erase(eraseQ.front());
        eraseQ.pop();
    }
	
    // Update the timeRemaining variable for running job
    /** if(runningJob->isRunning())
		runningJob->decrementTimeRemaining(p[5]-runningJob->getStartingTimeExecution());
	**/
}

/*
**                assignCorrectTImeQuantum()
**                   by Michael Figueroa
**
**    This function gets called inside dispatcher. If TIME_SLICE
**    is smaller than time remaining the job then it returns TIME_SLICE
**    else it returns the time remaining for the job
*/
long assignCorrectTimeQuantum()
{
    // If the timeRemaining is less than TIME_SLICE
	// return the difference. Else, return TIME_SLICE
    if((runningJob->getTimeRemaining() - TIME_SLICE) < 0)
        return runningJob->getTimeRemaining();
	else
        return TIME_SLICE;
}

/*
**                      longTermSch(0)
**             by Rakib Hasan and Michael Figueroa
**
**    This function gets called at whenever a terminating job's
**    program space is put back into the memory table. It goes
**    through each member of longTermSch map and finds a job that
**    can be put into memory. If it can be put into memory then
**    we move the job to the shortTermSch
*/
void longTermScheduler()
{
	multimap< long, PCB, greater<long> >::iterator it;
	queue< multimap< long, PCB, greater<long> >::iterator > eraseits;
	pair<long,long> freeSpace;
	
	for(it = longTermSch.begin(); it != longTermSch.end(); it++) {   // Find space for a job located in LTS
     	freeSpace = memManager.findSpace((it->second)[job_size]);
    		if(freeSpace.first != 0) {   
				// If space is found, push to STS
                (it->second).setJobAddr(freeSpace.second);
                shortTermSch.push((it->second));
				
                memManager.insertInTable(pair<long,long>(freeSpace.first - (it->second)[job_size],
										freeSpace.second + (it->second)[job_size]));
				
                memManager.defragment();
                eraseits.push(it);
            }
	}
	
	if(!eraseits.empty())
		while(!eraseits.empty()) {
            longTermSch.erase(eraseits.front());
            eraseits.pop();
        }
}


void moveJobPtr()
{
	if(cpuReadyQueue.size() > 1)
		if(runningJob  == --cpuReadyQueue.end())
			runningJob = cpuReadyQueue.begin();
		else
			runningJob++;
}

void testFunc(int val)
{
    switch(val) {
		case 7:		cout << "INSIDE CRINT" << endl; 	break;
		case 8:		cout << "INSIDE DSKINT" << endl; 	break;
		case 9:		cout << "INSIDE DRMINT" << endl; 	break;
		case 10:	cout << "INSIDE TRO" << endl; 		break;
		case 11: 	cout << "INSIDE SVC" << endl; 		break;
    }
	
    cout << "===================== Value in CPU RQ ========================" << endl;
    if(cpuReadyQueue.empty())
        cout << "CPU Queue Empty" << endl << endl;
		
    list<PCB>::iterator it;
	
    for(it = cpuReadyQueue.begin(); it != cpuReadyQueue.end(); it++) {
        cout << "Job number: " << (*it)[job_number] << endl;
        cout << "Job Address: " << (*it)[core_addr] << endl;
        cout << "Blocked: " << it->isBlocked() << endl;
        cout << "Terminated: " << it->isTerminated() << endl;
        cout << "IO count: " << it->getIOcnt() << endl;
        cout << "PendingIO: " << it->isPendingIO() << endl;
        cout << "Time Remaining: " << it->getTimeRemaining() << endl;
        cout << "Starting Time Execution: " << it->getStartingTimeExecution() << endl;
        cout << "Job Running: " << it->isRunning() << endl;
        cout << endl;
    }
	
    cout << "===================== Value in IO Queue ========================" << endl;
    if(ioQueue.empty())
        cout << "IO Queue Empty" << endl << endl;
		
    for(it = ioQueue.begin(); it != ioQueue.end(); it++)
		cout << "Job Number: " << (*it)[job_number] << endl;
		
    cout << endl;
	
    cout << "===================== Value in LTS ========================" << endl;
    if(longTermSch.empty())
        cout << "LTS Empty" << endl << endl;
		
    multimap<long, PCB, greater<long> >::iterator imp;
	
    for(imp = longTermSch.begin(); imp != longTermSch.end(); imp++)
		cout << "Job Number" << (imp->second)[job_number] << endl;
		
    cout << endl;
}
