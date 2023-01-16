#include <cstdlib>
#include <cstdio>
#include <cinttypes>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#define ADDRCELLSIZE 8
#define THREADCELLSIZE 4
#define VAMOSDR_COUNT_POTENTIAL_RACES

using namespace std;

enum class ActionType {
	ATRead, ATWrite, ATLock, ATUnlock, ATHappensIn, ATHappensOut, ATAlloc, ATFree, ATFork, ATJoin, ATSkipStart, ATSkipEnd, ATDone
};

typedef union {
	struct { intptr_t addr; } read;
	struct { intptr_t addr; } write;
	struct { intptr_t addr; } lock;
	struct { intptr_t addr; } unlock;
	struct { intptr_t addr; size_t size; } alloc;
	struct { intptr_t addr; } free;
	struct { int newthreadid; } fork;
	struct { int threadid; } join;
} Action;

void print_action(ActionType type, Action &act)
{
	switch(type)
	{
		case ActionType::ATRead:
		printf("read(%li)",act.read.addr);
		break;
		case ActionType::ATWrite:
		printf("write(%li)",act.write.addr);
		break;
		case ActionType::ATLock:
		printf("lock(%li)",act.lock.addr);
		break;
		case ActionType::ATUnlock:
		printf("unlock(%li)",act.unlock.addr);
		break;
		case ActionType::ATHappensIn:
		printf("happensIn(%li)",act.lock.addr);
		break;
		case ActionType::ATHappensOut:
		printf("happensOut(%li)",act.unlock.addr);
		break;
		case ActionType::ATAlloc:
		printf("alloc(%li, %lu)",act.alloc.addr, act.alloc.size);
		break;
		case ActionType::ATFree:
		printf("free(%li)",act.free.addr);
		break;
		case ActionType::ATFork:
		printf("fork(%i)",act.fork.newthreadid);
		break;
		case ActionType::ATJoin:
		printf("join(%i)",act.join.threadid);
		break;
		case ActionType::ATSkipStart:
		printf("skipstart()");
		break;
		case ActionType::ATSkipEnd:
		printf("skipend()");
		break;
		case ActionType::ATDone:
		printf("done()");
		break;
	}
}

class Cell {
	public:
	Cell * next;
	size_t refcount;
	uint64_t timestamp;
	int threadid;
	ActionType type;
	Action action;
};

class ThreadInfo {
	public:
	uint64_t lastHappensOut;
	uint64_t lastHappensIn;
	uint64_t lastSkipStart;
	uint64_t lastSkipEnd;
	set<intptr_t> locks;
};

Cell * head=new Cell();
Cell * tail=head;
size_t cellcount=0;
uint64_t racecount=0;
uint64_t potentialracecount=0;

extern "C" uint64_t GetRaceCount() { return racecount; }
extern "C" uint64_t GetPotentialRaceCount() { return potentialracecount; }

class Lockset
{
	public:
	unordered_set<intptr_t> addrs;
	unordered_set<int> threads;
};

class Info {
	public:
	Info(int owner, uint64_t timestamp):owner(owner),pos(0),timestamp(timestamp),alock(0),lockset()
	{
		pos=tail;
		lockset.threads.insert(owner);
	}
	Info(Info &a) =delete;
	Info(const Info &a) =delete;
	int owner;
	Cell * pos;
	uint64_t timestamp;
	intptr_t alock;
	Lockset lockset;
};

unordered_map<intptr_t, Info> Writes;
unordered_map<intptr_t, map<int, Info>> Reads;
unordered_map<intptr_t, int> LockThreads;
map<int, ThreadInfo> threads;

void print_lockset(Lockset &ls, bool printNewline=true)
{
	printf("Lockset %p = (addrs : {", (void*)&ls);
	bool seenfirst=false;
	for(auto addr : ls.addrs)
	{
		if(seenfirst)
		{
			printf(", ");
		}
		printf("%p", (void*)addr);
		seenfirst=true;
	}
	printf("}, threads: {");
	seenfirst=false;
	for(auto thrd : ls.threads)
	{
		if(seenfirst)
		{
			printf(", ");
		}
		printf("%i", thrd);
		seenfirst=true;
	}
	printf("})");
	if(printNewline)
	{
		printf("\n");
	}
}

void print_cell(Cell &cell, bool printRest=false)
{
	printf("Cell %p = (ts : %lu, thrd: %i, act: ", (void*)&cell, cell.timestamp, cell.threadid);
	print_action(cell.type, cell.action);
	printf(")");
	if(printRest&&cell.next!=0&&cell.next!=tail)
	{
		printf("; ");
		print_cell(*cell.next, true);
	}
	else
	{
		printf("\n");
	}
}

void print_info(Info &info, bool printCells=false)
{
	printf("Info %p = (owner: %i, alock: %li, ",(void*)&info, info.owner, info.alock);
	print_lockset(info.lockset,false);
	printf(")");
	if(printCells)
	{
		printf("; ");
		if(info.pos!=0&&info.pos!=tail)
		{
			print_cell(*info.pos,true);
		}
		else
		{
			printf("\n");
		}
	}
	else
	{
		printf("\n");
	}
}

void cell_rc_inc(Cell* cell)
{
	cell->refcount++;
}
void cell_rc_dec(Cell* cell)
{
	cell->refcount--;
	if(cell->refcount<=0)
	{
		if(cellcount>10000&&head->refcount==0)
		{
			while(head->refcount==0&&head->next!=0)
			{
				Cell* last=head;
				head=head->next;
				cellcount--;
				free(last);
			}
		}
	}
}

inline void enqueue_sync_event(int thrd, ActionType tp, Action &a)
{
	tail->threadid=thrd;
	tail->action=a;
	tail->type=tp;
	tail->next=new Cell();
	tail=tail->next;
	cellcount++;
}

inline bool thread_holds_lock(int thrd, intptr_t lock)
{
	auto posn = LockThreads.find(lock);
	if(posn!=LockThreads.end())
	{
		return (*posn).second==thrd;
	}
	return false;
}

void apply_lockset_rules(Lockset &ls, Cell **pos1r, Cell *pos2, int owner2, ActionType action, intptr_t addr, int otherthread, uint64_t orig_timestamp)
{
	Cell* pos1=*pos1r;
	Cell* origpos1=pos1;
	while(pos1!=pos2)
	{
		switch(pos1->type)
		{
			case ActionType::ATLock:
			case ActionType::ATHappensIn:
			{
				if(ls.addrs.find(pos1->action.lock.addr)!=ls.addrs.end())
				{
					ls.threads.insert(pos1->threadid);
				}
			}
			break;
			case ActionType::ATUnlock:
			case ActionType::ATHappensOut:
			{
				if(ls.threads.find(pos1->threadid)!=ls.threads.end())
				{
					ls.addrs.insert(pos1->action.unlock.addr);
				}
			}
			break;
			case ActionType::ATFork:
			{
				if(ls.threads.find(pos1->threadid)!=ls.threads.end())
				{
					ls.threads.insert(pos1->action.fork.newthreadid);
				}
			}
			break;
			case ActionType::ATJoin:
			{
				if(ls.threads.find(pos1->action.join.threadid)!=ls.threads.end())
				{
					ls.threads.insert(pos1->threadid);
				}
			}
			break;
			case ActionType::ATDone:
			{
			}
			break;
		}
		pos1=pos1->next;
		*pos1r=pos1;
	}
	cell_rc_inc(pos1);
	cell_rc_dec(origpos1);
	switch(action)
	{		
		case ActionType::ATRead:
		{
			if((!(ls.addrs.empty()&&ls.threads.empty()))&&ls.threads.find(owner2)==ls.threads.end())
			{
				auto &tinfo=threads[owner2];
				auto &otinfo=threads[otherthread];
				if((otinfo.lastSkipStart>orig_timestamp&&tinfo.lastHappensIn>otinfo.lastSkipStart) || (tinfo.lastSkipEnd>orig_timestamp&&tinfo.lastHappensOut>tinfo.lastSkipEnd))
				{
					#ifdef VAMOSDR_COUNT_POTENTIAL_RACES
					fprintf(stderr, "Potential data race: Thread %i read to %p, potentially without synchronizing with thread %i (relevant events skipped)\n", owner2, (void*)addr, otherthread);
					potentialracecount++;
					#endif
				}
				else
				{
					fprintf(stderr, "Found data race: Thread %i read from %p without synchronizing with thread %i\n", owner2, (void*)addr, otherthread);
					racecount++;
				}
			}
			// else
			// {
			// 	printf("read is fine\n");
			// }
			ls.addrs.clear();
			if(ls.threads.size()!=1||(*ls.threads.begin())!=owner2)
			{
				ls.threads.clear();
				ls.threads.insert(owner2);
			}
		}
		break;
		case ActionType::ATWrite:
		{
			if((!(ls.addrs.empty()&&ls.threads.empty()))&&ls.threads.find(owner2)==ls.threads.end())
			{
				auto &tinfo=threads[owner2];
				auto &otinfo=threads[otherthread];
				if((otinfo.lastSkipStart>orig_timestamp&&tinfo.lastHappensIn>otinfo.lastSkipStart) || (tinfo.lastSkipEnd>orig_timestamp&&tinfo.lastHappensOut>tinfo.lastSkipEnd))
				{
					#ifdef VAMOSDR_COUNT_POTENTIAL_RACES
					fprintf(stderr, "Potential data race: Thread %i wrote to %p, potentially without synchronizing with thread %i (relevant events skipped)\n", owner2, (void*)addr, otherthread);
					potentialracecount++;
					#endif
				}
				else
				{
					fprintf(stderr, "Found data race: Thread %i wrote to %p without synchronizing with thread %i\n", owner2, (void*)addr, otherthread);
					racecount++;
				}
			}
			// else
			// {
			// 	printf("write is fine\n");
			// }
			ls.addrs.clear();
			if(ls.threads.size()!=1||(*ls.threads.begin())!=owner2)
			{
				ls.threads.clear();
				ls.threads.insert(owner2);
			}
		}
		break;
	}
}

void check_happens_before(Info &info1, Info &info2, ActionType action, intptr_t addr)
{
	// print_info(info1, true);
	// print_info(info2, true);
	// if(info1->xact&&info2->xact)
	// {
	// 	return;
	// }
	if((info1.owner!=info2.owner)&&(!thread_holds_lock(info2.owner,info1.alock)))
	{
		apply_lockset_rules(info1.lockset, &info1.pos, info2.pos, info2.owner, action, addr, info1.owner, info1.timestamp);
		info2.alock=0;
		auto tinfo=threads[info1.owner];
		auto posn=tinfo.locks.begin();
		if(posn!=tinfo.locks.end())
		{
			info2.alock=*posn;
		}
	}
}


extern "C" void monitor_handle_read(int tid, uint64_t timestamp, intptr_t addr)
{
	auto &addrmap = Reads[addr];
	auto entry = addrmap.find(tid);
	cell_rc_inc(tail);
	if(entry==addrmap.end())
	{
		entry=addrmap.try_emplace(tid,tid,timestamp).first;
	}
	else
	{
		entry->second.alock=0;
		entry->second.owner=tid;
		entry->second.lockset.addrs.clear();
		entry->second.lockset.threads.clear();
		entry->second.lockset.threads.insert(tid);
		entry->second.timestamp=timestamp;
		cell_rc_dec(entry->second.pos);
		entry->second.pos=tail;
	}
	auto wentry=Writes.find(addr);
	if(wentry!=Writes.end())
	{
		check_happens_before(wentry->second,entry->second, ActionType::ATRead, addr);
	}
}
extern "C" void monitor_handle_read_many(int tid, uint64_t timestamp, intptr_t addr, size_t bytes)
{
	for(size_t i=0;i<bytes;i++)
	{
		monitor_handle_read(tid,timestamp,addr+i);
	}
}

extern "C" void monitor_handle_write(int tid, uint64_t timestamp, intptr_t addr)
{
	// printf("MWRITE! %p\n", addr);
	Info info(tid, timestamp);
	cell_rc_inc(tail);
	// print_cell(*head,true);
	auto rinfo = Reads.find(addr);
	if(rinfo!=Reads.end())
	{
		for(auto &tlp : threads)
		{
			auto trinfo = rinfo->second.find(tlp.first);
			if(trinfo!=rinfo->second.end())
			{
				check_happens_before(trinfo->second, info, ActionType::ATWrite, addr);
			}
		}
	}
	// else
	// {
	// 	printf("MWRITE: no reads found, %p\n", addr);
	// }
	auto winfo = Writes.find(addr);
	if(winfo!=Writes.end())
	{
		check_happens_before(winfo->second, info, ActionType::ATWrite, addr);
		cell_rc_dec(winfo->second.pos);
		winfo->second=std::move(info);
	}
	else
	{
		Writes.try_emplace(addr, tid, timestamp);
		// printf("MWRITE: no writes found, %p\n", addr);
	}
	if(rinfo!=Reads.end())
	{
		for(auto tlp=rinfo->second.begin();tlp!=rinfo->second.end();tlp++)
		{
			cell_rc_dec(tlp->second.pos);
		}
		rinfo->second.clear();
	}
	Reads.extract(addr);
}
extern "C" void monitor_handle_write_many(int tid, uint64_t timestamp, intptr_t addr, size_t bytes)
{
	for(size_t i=0;i<bytes;i++)
	{
		monitor_handle_write(tid,timestamp,addr+i);
	}
}

extern "C" void monitor_handle_happensin(int tid, uint64_t timestamp, intptr_t addr)
{
	Action a;
	a.lock.addr=addr;
	enqueue_sync_event(tid, ActionType::ATHappensIn, a);
	threads[tid].lastHappensIn=timestamp;
}

extern "C" void monitor_handle_happensout(int tid, uint64_t timestamp, intptr_t addr)
{
	Action a;
	a.unlock.addr=addr;
	enqueue_sync_event(tid, ActionType::ATHappensOut, a);
	threads[tid].lastHappensOut=timestamp;
}

extern "C" void monitor_handle_lock(int tid, uint64_t timestamp, intptr_t addr)
{
	Action a;
	a.lock.addr=addr;
	LockThreads[addr]=tid;
	threads[tid].locks.insert(addr);
	enqueue_sync_event(tid, ActionType::ATLock, a);
	threads[tid].lastHappensIn=timestamp;
}

extern "C" void monitor_handle_unlock(int tid, uint64_t timestamp, intptr_t addr)
{
	Action a;
	a.unlock.addr=addr;
	LockThreads.erase(addr);
	threads[tid].locks.erase(addr);
	enqueue_sync_event(tid, ActionType::ATUnlock, a);
	threads[tid].lastHappensOut=timestamp;
}

extern "C" void monitor_handle_skip_start(int tid, uint64_t timestamp)
{
	Action a;
	threads[tid].lastSkipStart=timestamp;
}
extern "C" void monitor_handle_skip_end(int tid, uint64_t timestamp)
{
	Action a;
	threads[tid].lastSkipEnd=timestamp;
}
extern "C" void monitor_handle_alloc(int tid, uint64_t timestamp, intptr_t addr, size_t size)
{
	Action a;
	a.alloc.addr=addr;
	a.alloc.size=size;
	enqueue_sync_event(tid, ActionType::ATAlloc, a);
}

extern "C" void monitor_handle_free(int tid, uint64_t timestamp, intptr_t addr)
{
	Action a;
	a.free.addr=addr;
	enqueue_sync_event(tid, ActionType::ATFree, a);
}
extern "C" void monitor_handle_fork(int tid, uint64_t timestamp, int otherthread)
{
	Action a;
	a.fork.newthreadid=otherthread;
	threads.erase(otherthread);
	enqueue_sync_event(tid, ActionType::ATFork, a);
	threads[otherthread].lastHappensIn=timestamp;
	threads[tid].lastHappensOut=timestamp;
}

extern "C" void monitor_handle_join(int tid, uint64_t timestamp, int otherthread)
{
	Action a;
	a.join.threadid=otherthread;
	enqueue_sync_event(tid, ActionType::ATJoin, a);
	threads[otherthread].lastHappensOut=timestamp;
	threads[tid].lastHappensIn=timestamp;
}

extern "C" void monitor_handle_done(int tid, uint64_t timestamp)
{
	Action a;
	//enqueue_sync_event(tid, ActionType::ATDone, a);
	bool eraseNext=false;
	intptr_t eraseKey=0;
	for(auto &entry : LockThreads)
	{
		if(eraseNext)
		{
			LockThreads.erase(eraseKey);
			eraseNext=false;
		}
		if(entry.second==tid)
		{
			eraseKey=entry.first;
			eraseNext=true;
			break;
		}
	}
	if(eraseNext)
	{
		LockThreads.erase(eraseKey);
	}
}
