/*******     Memory Manager     *******/


#include "os.h"

/*
**			MemoryManager(0)
**	-Inputs:
**		none
**	-Description:
**		Default constructor for MemoryManager
**		It inserts the first entry for the free space
**		table
**		The first entry has size 100 and starts at addr
**		0
**	-Output
**		none
*/

MemoryManager::MemoryManager() 
{
	pair<long,long> firstEntry(100,0);
	fsTable.insert(firstEntry);
}


/*
**			insertInTable(2)
**	-Inputs:
**		int size, addr
**	-Description:
**		It creates a new entry for the free space table
**		and inserts it into the table
**	-Output:
**		none
*/

void MemoryManager::insertInTable(pair<long,long> newEntry) 
{
	fsTable.insert(newEntry);
}


/*
**			findSpace(1)
**	-Inputs:
**		int jobSize
**	-Descriptions:
**		It looks for a entry in the free space table
**		that contains a size that is bigger or equal
**		to jobSize
**	-Outputs:
**		if it finds a entry then it returns the entry
**		if not then it returns an entry with 0's as the
**		values
**
*/

pair<long, long> MemoryManager::findSpace(long jobSize)
{
	pair<long,long> temp;
	multimap<long,long,less<long> >::iterator it;
	
	if(fsTable.empty()) {
		return pair<long, long>(0,0);   //--->Default constructor always has a value within the map, would it ever be empty?
	}
	
	for(it = fsTable.begin(); it != fsTable.end(); it++) {
		if(jobSize <= it->first) {
			temp = *it;
			fsTable.erase(it);
			return temp;
		}
	}
	
	return pair<long,long>(0,0);
}


/*
**			defragment(0)
**	-Inputs:
**		none
**	-Description:
**		This function will combine fragments that
**		are adjacent to each other in order to reduce
**		fragmentation within the free space table
**	-Output:
**		none
*/

void MemoryManager::defragment()
{
	multimap<long,long,less<long> > temp;
	multimap<long,long,less<long> >::iterator it;
	list<long> numStr;
	long size = 0, addr = 0, newSize = 0;

	for(it = fsTable.begin(); it != fsTable.end(); it++)
		temp.insert(pair<long,long>(it->second,it->first));
	
	fsTable.clear();
	
	for(it = temp.begin(); it != temp.end(); it++) {
		numStr.push_back(it->first);
		numStr.push_back(it->second);
	}
	
	while(!numStr.empty()) 
	{
		start:addr = numStr.front();
		numStr.pop_front();
		size = numStr.front();
		numStr.pop_front();

		if(((addr + size) == numStr.front()) && !numStr.empty()) {
			numStr.pop_front();
			newSize = size + numStr.front();
			numStr.pop_front();
			numStr.push_front(newSize);
			numStr.push_front(addr);
			goto start;
		} 
		else {
			fsTable.insert(pair<long,long>(size,addr));
		}
	}
}


/*******     End Memory Manager     *******/
