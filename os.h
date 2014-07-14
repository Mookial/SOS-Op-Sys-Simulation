#ifndef OS_H
#define OS_H

#include <map>
#include <queue>
#include <functional>
#include <list>
#include <iostream>

using namespace std;

/***************************
**     Memory Manager     **
****************************/

class MemoryManager {
	private:
		multimap<long, long , less<long> > fsTable; // Free-Space Table
	public:
		MemoryManager();
		void insertInTable(pair<long, long>); // Inserts the job and it's size into table
		pair<long, long> findSpace(long); // Check for free space for a given job size
		void defragment(); // Defrag table to check for adjacent free-space chunks
};


/****************************
**     PCB (Job class)     **
*****************************/

class PCB {
	private:
		// Job properties are stored here
     	long jobNumber;
		long priority;
		long jobSize;
		long timeRemaining;
		long timeOfArrival;
		long jobAddr;
		long pendingIOcnt;
		long startingTimeExec;
		long ioCnt;
		
		bool pendingIO;
		bool blocked;
		bool doingIO;
		bool terminated;
		bool jobRunning;
     public:
		// Constructor that intilaizes all values
		PCB(long jNum = -1, long p = -1, long jSize = -1, long tR = -1, long tA = -1, long jAddr = -1):
		jobNumber(jNum), priority(p), jobSize(jSize), timeRemaining(tR), timeOfArrival(tA), jobAddr(jAddr),
		blocked(false), doingIO(false), pendingIO(false), terminated(false),startingTimeExec(0), ioCnt(0),
		jobRunning(false)
		{}
		
		// Destructor
		~PCB() {}
		
		// Copy Constructor
		PCB(const PCB &rObj) {
		    this->jobNumber = rObj.jobNumber;
			this->priority = rObj.priority;
			this->jobSize = rObj.jobSize;
			this->timeRemaining = rObj.timeRemaining;
			this->timeOfArrival = rObj.timeOfArrival;
			this->jobAddr = rObj.jobAddr;
			this->blocked = rObj.blocked;
            this->ioCnt = rObj.ioCnt;
            this->doingIO = rObj.doingIO;
            this->blocked = rObj.blocked;
            this->pendingIO = rObj.pendingIO;
            this->terminated = rObj.terminated;
            this->startingTimeExec = rObj.startingTimeExec;
            this->jobRunning = rObj.jobRunning;
		}
		
		// Overloaded Operators
		void operator =(PCB & rObj) {
			this->jobNumber = rObj[1];
			this->priority = rObj[2];
			this->jobSize = rObj[3];
			this->timeRemaining = rObj[4];
			this->timeOfArrival = rObj[5];
			this->jobAddr = rObj[6];
			this->blocked = rObj.blocked;
            this->doingIO = rObj.doingIO;
            this->pendingIO = rObj.pendingIO;
            this->terminated = rObj.terminated;
            this->startingTimeExec = rObj.startingTimeExec;
            this->ioCnt = rObj.ioCnt;
            this->jobRunning = rObj.jobRunning;
		}
		bool operator ==(PCB &rObj) { return (rObj[1] == (*this)[1] ? true: false); }
		bool operator !=(PCB &rObj) { return !((*this) == rObj); }

		// Overloaded Subscript Operator
       	long & operator[](long i) {
			switch(i) {
				case 1: return jobNumber; break;
				case 2: return priority; break;
				case 3: return jobSize;	break;
				case 4: return timeRemaining; break;
				case 5: return timeOfArrival; break;
				case 6: return jobAddr; break;
				default: cerr<<"Out of bounds error"<<endl;
			}
		}
		
		// Accessors
		bool isBlocked() { return blocked; }
        long getPriority() { return priority; } 
        long getJobAddress() {return jobAddr; } 
        long getJobNumber() { return jobNumber; } 
        long getJobSize() { return jobSize; } 
        long getTimeRemaining() { return timeRemaining; }
        long getTimeOfArrival() { return timeOfArrival; }
        long getPendingIOcnt() { return pendingIOcnt; }
        long getStartingTimeExecution() { return startingTimeExec; } // Returns time job started running on CPU
        long getIOcnt() { return ioCnt; }
        bool isDoingIO() { return doingIO; }
        bool isPendingIO() {return pendingIO; }
        bool isTerminated() { return terminated; }
        bool isRunning() { return jobRunning; }
		
		//Mutators
		void setBlocked(bool value) { blocked = value; }
		void decrementTimeRemaining(int timeRan) { timeRemaining -= timeRan; } // Process requested to be unblocked
		void incrementTimeRemaining(int timeBlck) { timeRemaining += timeBlck; }
		void setJobAddr(long addr) {jobAddr = addr; }
        void setJobNumber(long jNum) { jobNumber = jNum ; }
		void setDoingIO(bool isdio) { doingIO = isdio; }
		void incIOcnt() { ioCnt++; }
		void decIOcnt() { ioCnt--; }
		void setPendingIO(bool pIO) { pendingIO = pIO; }
		void setStartingTimeExecution(long tm) {startingTimeExec = tm; } // Set starting time
		void setTerminated(bool term) { terminated = term; }
        void setJobRunning(bool rn) { jobRunning = rn; }
};

#endif
