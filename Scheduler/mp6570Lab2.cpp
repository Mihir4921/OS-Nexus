// NAME: MIHIR PRAJAPATI
// NETID: mp6570
// OS ASSIGNMENT LAB 2

#include <iostream>
#include <cctype>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

int randOffset = 0, QUANTUM = 10000, maxPrio = 4;
bool VERBOSE = false, EVENTEXEC = false, EVENTQBEFAF = false, PREEMPTDETAIL = false;
double totIOTime = 0, overallCPUTimeUsed = 0, totTATime = 0, totCPUWait = 0, finalTime;
vector<int> randomNums;

typedef enum process_states
{
    CREATED,
    READY,
    RUNNING,
    BLOCKED,
    COMPLETED
} PROC_STATE_TYPES;

enum process_transitions
{
    TRANS_TO_READY,
    TRANS_TO_PREEMPT,
    TRANS_TO_RUN,
    TRANS_TO_BLOCK
};

class Process
{
public:
    int arrivalTime;
    int totalTimeRequired;
    int CPUBurst;
    int IOBurst;
    int PID;

    int staticPriority;
    int dynamicPriority;

    int burstRemaining;

    int timeRemaining;
    int turnaroundTime;
    int finishTime;
    int timeInIO;    // time in blocked state
    int timeWaiting; // time in ready state
    PROC_STATE_TYPES currProcessState;
    int currStateStartTime;

    Process(int pid, int arrTime, int timeReq, int cpuBurst, int ioBurst, int staticPrio)
        : PID(pid),
          arrivalTime(arrTime),
          totalTimeRequired(timeReq),
          CPUBurst(cpuBurst),
          IOBurst(ioBurst),
          staticPriority(staticPrio),
          dynamicPriority(staticPrio - 1),
          currProcessState(process_states::CREATED),
          burstRemaining(0),
          timeRemaining(timeReq),
          turnaroundTime(0),
          finishTime(0),
          timeInIO(0), // time in blocked state
          timeWaiting(0),
          currStateStartTime(arrTime)
    {
    }
};

// DES SUPPORT START

class Event
{
public:
    int eventTime;
    Process *proc;
    process_transitions currTransition;

    Event(int evTime, Process *procArg, process_transitions transArg)
        : eventTime(evTime),
          proc(procArg),
          currTransition(transArg) {}
};

// PLACED HERE AS CAN ONLY BE DECLARED AFTER EVENT CLASS DECLARATION
list<Event *> eventQueue;

void addEvent(Event *ev)
{
    if (eventQueue.empty())
    {
        eventQueue.push_back(ev);
        return;
    }
    auto it = eventQueue.begin();
    while (it != eventQueue.end())
    {
        // SHOULD BE >= or >
        // CHECK IF POST-INCREMENET WORKS
        if ((*it)->eventTime > ev->eventTime)
        {
            // LOOKS DOUBTFUL AS POINTERS MIGHT NOT BE CORRECT
            eventQueue.insert(it, ev);
            return;
        }
        ++it;
    }
    eventQueue.push_back(ev);
}

Event *getEvent()
{
    if (eventQueue.empty())
    {
        return NULL;
    }
    Event *ev = eventQueue.front(); // *eventQueue.front()
    eventQueue.pop_front();
    return ev;
}

void initializeEventQueue(vector<Process *> procArray)
{
    // CHECK CORRECT USAGE OF POINTERS FOR PROCARRAY
    for (auto proc : procArray)
    {
        Event *ev = new Event(proc->arrivalTime, proc, process_transitions::TRANS_TO_READY);
        // CHECK IF POST-INCREMENET WORKS
        eventQueue.push_back(ev);
    }
}

int getNextEventTime()
{
    if (eventQueue.empty())
        return -1;
    return eventQueue.front()->eventTime;
}

Event *findEventForProcess(Process *proc)
{
    for (auto it : eventQueue)
    {
        if (it->proc->PID == proc->PID)
        {
            return it;
        }
    }
    return NULL;
}

void deleteGivenEvent(int pid)
{
    auto it = eventQueue.begin();
    while (it != eventQueue.end())
    {
        if ((*it)->proc->PID == pid)
        {
            eventQueue.erase(it);
            return;
        }
        it++;
    }
}

// DES SUPPORT END

class Scheduler
{
public:
    list<Process *> runQueue;
    string type;

    virtual void addProcess(Process *proc) = 0;
    virtual Process *getNextProcess() = 0;
    virtual bool isPreemptive()
    {
        return false;
    }
};

class FCFS : public Scheduler
{
public:
    FCFS()
    {
        type = "FCFS";
    }

    void addProcess(Process *proc)
    {
        runQueue.push_back(proc);
    }

    Process *getNextProcess()
    {
        if (!runQueue.empty())
        {
            Process *receivedProc = runQueue.front();
            runQueue.pop_front();
            return receivedProc;
        }
        return NULL;
    }
};

class LCFS : public Scheduler
{
public:
    LCFS()
    {
        type = "LCFS";
    }

    void addProcess(Process *proc)
    {
        runQueue.push_back(proc);
    }

    Process *getNextProcess()
    {
        if (!runQueue.empty())
        {
            Process *receivedProc = runQueue.back();
            runQueue.pop_back();
            return receivedProc;
        }
        return NULL;
    }
};

class RoundRobin : public Scheduler
{
public:
    RoundRobin()
    {
        type = "RR";
    }
    void addProcess(Process *proc)
    {
        proc->dynamicPriority = proc->staticPriority - 1;
        runQueue.push_back(proc);
    }

    Process *getNextProcess()
    {
        if (!runQueue.empty())
        {
            Process *receivedProc = runQueue.front();
            runQueue.pop_front();
            return receivedProc;
        }
        return NULL;
    }
};

class SRTF : public Scheduler
{
public:
    SRTF()
    {
        type = "SRTF";
    }
    void addProcess(Process *proc)
    {
        if (runQueue.empty())
        {
            runQueue.push_back(proc);
            return;
        }

        auto it = runQueue.begin();
        while (it != runQueue.end())
        {
            if ((*it)->timeRemaining > proc->timeRemaining)
            {
                runQueue.insert(it, proc);
                return;
            }
            it++;
        }
        runQueue.push_back(proc);
    }

    Process *getNextProcess()
    {
        if (!runQueue.empty())
        {
            Process *receivedProc = runQueue.front();
            runQueue.pop_front();
            return receivedProc;
        }
        return NULL;
    }
};

class PRIO : public Scheduler
{
public:
    vector<list<Process *>> activeQueue;
    vector<list<Process *>> expiredQueue;

    PRIO()
    {
        type = "PRIO";
        for (int i = 0; i < maxPrio; i++)
        {
            activeQueue.push_back(list<Process *>());
            expiredQueue.push_back(list<Process *>());
        }
    }

    void addProcess(Process *proc)
    {
        if (proc->dynamicPriority < 0)
        {
            proc->dynamicPriority = proc->staticPriority - 1;
            expiredQueue[proc->dynamicPriority].push_back(proc);
        }
        else
        {
            activeQueue[proc->dynamicPriority].push_back(proc);
        }
    }

    Process *getNextProcess()
    {
        bool activeEmptyFlag = true, expiredEmptyFlag = true;
        for (int i = 0; i < activeQueue.size(); i++)
        {
            if (!activeQueue[i].empty())
            {
                activeEmptyFlag = false;
            }
            if (!expiredQueue[i].empty())
            {
                expiredEmptyFlag = false;
            }
        }

        if (activeEmptyFlag && expiredEmptyFlag)
        {
            return NULL;
        }
        if (activeEmptyFlag)
        {
            activeQueue.swap(expiredQueue);
        }

        for (int i = activeQueue.size() - 1; i > -1; i--)
        {
            if (!activeQueue[i].empty())
            {
                Process *receivedProc = activeQueue[i].front();
                activeQueue[i].pop_front();
                return receivedProc;
            }
        }

        // If none of the above occur then there's a problem, so return NULL.
        return NULL;
    }
};

class PREPRIO : public PRIO
{
public:
    PREPRIO() : PRIO()
    {
        type = "PREPRIO";
    }

    bool isPreemptive()
    {
        return true;
    }
};

int getRandom(int remainder)
{
    if (randOffset == randomNums.size())
        randOffset = 0;
    return (1 + (randomNums[randOffset++] % remainder));
}

void Simulation(Scheduler *scheduler)
{
    Event *mainEvent;
    bool CALL_SCHEDULER = false;
    Process *CURRENT_RUNNING_PROCESS = NULL;
    int blockedProcsNum = 0;
    int overallIOStart;
    while ((mainEvent = getEvent()))
    {
        process_transitions eventTransitionObtained = mainEvent->currTransition;
        Process *curProc = mainEvent->proc;
        int CURRENT_TIME = mainEvent->eventTime;
        int timeInPrevState = CURRENT_TIME - curProc->currStateStartTime;
        curProc->currStateStartTime = CURRENT_TIME;

        switch (eventTransitionObtained)
        {
        case TRANS_TO_READY:
            if (curProc->currProcessState == BLOCKED)
            {
                curProc->timeInIO += timeInPrevState;
                curProc->dynamicPriority = curProc->staticPriority - 1;
                --blockedProcsNum;
                if (!blockedProcsNum)
                {
                    totIOTime += (CURRENT_TIME - overallIOStart);
                }
            }

            if (VERBOSE)
            {
                string previousStateString = "CREATED";
                if (curProc->currProcessState == BLOCKED)
                    previousStateString = "BLOCK";
                else if (curProc->currProcessState == RUNNING)
                    previousStateString = "RUNNG";
                cout << CURRENT_TIME << " " << curProc->PID << " " << timeInPrevState << ": " << previousStateString << " -> READY\n";
            }

            if (CURRENT_RUNNING_PROCESS != NULL && scheduler->isPreemptive())
            {
                Event *runningProcEvent = findEventForProcess(CURRENT_RUNNING_PROCESS);
                int runningProcEventTime = runningProcEvent->eventTime;
                int runningProcDynPrio = CURRENT_RUNNING_PROCESS->dynamicPriority;
                // cout << CURRENT_TIME << " Gets here\n";
                if (curProc->dynamicPriority > runningProcDynPrio && CURRENT_TIME < runningProcEventTime)
                {
                    // cout << CURRENT_TIME << " Gets here too\n";
                    deleteGivenEvent(CURRENT_RUNNING_PROCESS->PID);
                    if (runningProcEvent->currTransition == TRANS_TO_BLOCK)
                    {
                        CURRENT_RUNNING_PROCESS->timeRemaining -= CURRENT_RUNNING_PROCESS->burstRemaining;
                        CURRENT_RUNNING_PROCESS->burstRemaining = 0;
                    }
                    CURRENT_RUNNING_PROCESS->timeRemaining += (runningProcEventTime - CURRENT_TIME);
                    CURRENT_RUNNING_PROCESS->burstRemaining += (runningProcEventTime - CURRENT_TIME);
                    addEvent(new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT));
                }
            }

            curProc->currProcessState = READY;
            scheduler->addProcess(curProc);
            // cout << "Process added\n";
            CALL_SCHEDULER = true;
            break;
        case TRANS_TO_PREEMPT:
            // Main thing that's supposed to happen here is that due to preemption, the dyanmic priority of the
            // process in question is decreased by one.
            if (VERBOSE)
                cout << CURRENT_TIME << " " << curProc->PID << " " << timeInPrevState << ": RUNNG -> READY cb=" << curProc->burstRemaining << " rem=" << (curProc->timeRemaining) << " prio=" << curProc->dynamicPriority << endl;

            curProc->dynamicPriority--;
            scheduler->addProcess(curProc);

            CURRENT_RUNNING_PROCESS = NULL;
            CALL_SCHEDULER = true;
            break;
        case TRANS_TO_RUN:
            if (curProc->burstRemaining == 0)
            {
                curProc->burstRemaining = getRandom(curProc->CPUBurst);
            }
            curProc->burstRemaining = min(curProc->burstRemaining, curProc->timeRemaining);

            // The process is now effectively running so update parameters for the same
            curProc->currProcessState = RUNNING;
            // curProc->currStateStartTime = mainEvent->eventTime;
            curProc->timeWaiting += timeInPrevState;
            totCPUWait += timeInPrevState;

            // Print the log if 'v' is enabled
            if (VERBOSE)
                cout << CURRENT_TIME << " " << curProc->PID << " " << timeInPrevState << ": READY -> RUNNG cb=" << curProc->burstRemaining << " rem=" << (curProc->timeRemaining) << " prio=" << curProc->dynamicPriority << endl;

            // If remaining burst is less than the Quantum, then its burst is exhausted, and it goes to
            // IO/Blocked
            if (curProc->burstRemaining <= QUANTUM)
            {
                addEvent(new Event(CURRENT_TIME + (curProc->burstRemaining), curProc, TRANS_TO_BLOCK));
            }
            // If Quantum is less than the remaining burst time, then the process gets preempted
            // after running for the specific quantum time. So, the timeremaining and burstremaining are
            // updated.
            else
            {
                curProc->timeRemaining -= QUANTUM;
                curProc->burstRemaining -= QUANTUM;
                addEvent(new Event(CURRENT_TIME + QUANTUM, curProc, TRANS_TO_PREEMPT));
            }

            break;
        case TRANS_TO_BLOCK:
            // Since the process's burst is completed, it gets blocked, thus there will be no running processes
            // and the burst will be zero, and timeremaining will be updated.
            CURRENT_RUNNING_PROCESS = NULL;
            curProc->timeRemaining -= curProc->burstRemaining;
            curProc->burstRemaining = 0;
            // Handles the case where cpuBurst is exhausted and no CPUTime is required. So process is completed
            // and it doesn't go to blocked state
            if (curProc->timeRemaining <= 0)
            {
                curProc->currProcessState = COMPLETED;
                curProc->finishTime = CURRENT_TIME;
                curProc->turnaroundTime = CURRENT_TIME - curProc->arrivalTime;
                totTATime += curProc->turnaroundTime;
                finalTime = CURRENT_TIME;

                if (VERBOSE)
                    cout << CURRENT_TIME << " " << curProc->PID << " " << timeInPrevState << ": Done" << endl;
            }
            else
            {
                int ioBurst = getRandom(curProc->IOBurst);
                curProc->currProcessState = BLOCKED;

                // If there are no blocked processes currently, then it signifies the start of IO utilization
                // so the start time is recorded, and when there are no processes in blocked state, the event time
                // is subtracted with the below IO start, to get the IO utilization.

                // Print the log if 'v' is enabled
                if (VERBOSE)
                    cout << CURRENT_TIME << " " << curProc->PID << " " << timeInPrevState << ": RUNNG -> BLOCK  ib=" << ioBurst << " rem=" << (curProc->timeRemaining) << endl;

                if (blockedProcsNum == 0)
                {
                    overallIOStart = CURRENT_TIME;
                }
                ++blockedProcsNum;
                addEvent(new Event(CURRENT_TIME + ioBurst, curProc, TRANS_TO_READY));
            }
            CALL_SCHEDULER = true;
            break;
        }
        if (CALL_SCHEDULER)
        {
            if (getNextEventTime() == CURRENT_TIME)
            {
                continue;
            }
            CALL_SCHEDULER = false;
            if (CURRENT_RUNNING_PROCESS == NULL)
            {
                CURRENT_RUNNING_PROCESS = scheduler->getNextProcess();
                if (CURRENT_RUNNING_PROCESS == NULL)
                {
                    continue;
                }
                addEvent(new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, process_transitions::TRANS_TO_RUN));
                // cout << "Process Added: " << CURRENT_RUNNING_PROCESS->PID << " at time: " << CURRENT_TIME << endl;
            }
        }
    }
}

// PARSING FROM HERE

Scheduler *parseSchedulerArg(string schedArg)
{
    Scheduler *schedSpec;
    string schedType;
    switch (schedArg[0])
    {
    case 'F':
        schedType = "FCFS";
        schedSpec = new FCFS();
        break;
    case 'L':
        schedType = "LCFS";
        schedSpec = new LCFS();
        break;
    case 'S':
        schedType = "SRTF";
        schedSpec = new SRTF();
        break;
    case 'R':
        schedType = "RR";
        sscanf(schedArg.c_str(), "%*c%d", &QUANTUM);
        schedSpec = new RoundRobin();
        break;
    case 'P':
        if (schedArg.find(':') != string::npos)
            sscanf(schedArg.c_str(), "%*c%d:%d", &QUANTUM, &maxPrio); // found
        else
            sscanf(schedArg.c_str(), "%*c%d", &QUANTUM); // not found
        schedType = "PRIO";
        schedSpec = new PRIO();
        break;
    case 'E':
        if (schedArg.find(':') != string::npos)
            sscanf(schedArg.c_str(), "%*c%d:%d", &QUANTUM, &maxPrio); // found
        else
            sscanf(schedArg.c_str(), "%*c%d", &QUANTUM); // not found
        schedType = "PREPRIO";
        schedSpec = new PREPRIO();
        break;
    default:
        cout << "Error: Scheduler Type Incorrect/Not Specified." << endl;
        exit(0);
        break;
    }

    // CHECK CORRECT PARSING
    // cout << schedType << " " << QUANTUM << " " << maxPrio << endl;

    return schedSpec;
}

Scheduler *parseArgs(int argc, char **argv)
{
    string schedInitializeInfo;
    int c;
    while ((c = getopt(argc, argv, "vteps:")) != -1)
    {
        switch (c)
        {
        case 'v':
            VERBOSE = true;
            break;
        case 't':
            EVENTEXEC = true;
            break;
        case 'e':
            EVENTQBEFAF = true;
            break;
        case 'p':
            PREEMPTDETAIL = true;
            break;
        case 's':
            schedInitializeInfo = optarg;
            break;
        }
    }

    if (schedInitializeInfo.empty())
    {
        cout << "Error: Scheduler not specified." << endl;
        exit(0);
    }

    return parseSchedulerArg(schedInitializeInfo);
}

void parseRandomFile(string randFName)
{
    ifstream randFileObj(randFName);
    string totalRandNums;
    randFileObj >> totalRandNums;
    string randNum;
    while (randFileObj >> randNum)
    {
        randomNums.push_back(stoi(randNum));
    }
}

vector<Process *> parseProcessFile(string inputFName)
{
    ifstream inputFileObj(inputFName);
    int pid = 0;
    string arrTime, totCPUTime, cpuBurst, ioBurst;
    vector<Process *> inputProcessArray;
    while (inputFileObj >> arrTime >> totCPUTime >> cpuBurst >> ioBurst)
    {
        Process *proc = new Process(pid++, stoi(arrTime), stoi(totCPUTime), stoi(cpuBurst), stoi(ioBurst), getRandom(maxPrio));
        inputProcessArray.push_back(proc);

        // For CPU Util %age
        overallCPUTimeUsed += stoi(totCPUTime);
    }
    return inputProcessArray;
}

// TESTING FUNCTIONS START

void printProcessArray(vector<Process *> procArray)
{
    cout << "Printing Process Array" << endl;
    cout << "PID | Arr Time | Time Required | CPU Burst | IO Burst | Dynamic Prio | Proc State\n";
    for (auto it : procArray)
    {
        cout << it->PID << " " << it->arrivalTime << " " << it->totalTimeRequired << " " << it->CPUBurst << " " << it->IOBurst << " " << it->dynamicPriority << " " << it->currProcessState << endl;
    }
}

void printEventQueue()
{
    cout << "Printing Event Queue" << endl;
    cout << "Transition of Event to Execute | Event Time | Event PID | Event Proc State | Burst Rem | time in IO\n";
    for (auto it : eventQueue)
    {
        cout << it->currTransition << " " << it->eventTime << " " << it->proc->PID << " " << it->proc->currProcessState << " " << it->proc->burstRemaining << " " << it->proc->timeInIO << " " << endl;
    }
}

void printRunQueue(Scheduler *sched)
{
    cout << "Printing Scheduler Run Queue" << endl;
    for (auto proc : sched->runQueue)
    {
        cout << proc->PID << endl;
    }
}

void testModifyProcArray(vector<Process *> *procArray)
{
    // EXPERIMENT FUNCTION TO CHECK POINTER MODIFICATION
    for (auto it : *procArray)
    {
        it->PID = 1212;
    }
}

void printFinalProcessArray(vector<Process *> procArray)
{
    // cout << "Printing Final Process Status" << endl;
    //  cout << "PID | Arr Time | Time Required | CPU Burst | IO Burst | Dynamic Prio | Proc State\n";
    for (auto it : procArray)
    {
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", it->PID, it->arrivalTime,
               it->totalTimeRequired, it->CPUBurst, it->IOBurst, it->staticPriority, it->finishTime,
               it->turnaroundTime, it->timeInIO, it->timeWaiting);
    }
}

void printSummary(int procNum)
{
    double cpu_util = 100.0 * (overallCPUTimeUsed / finalTime);
    double io_util = 100.0 * (totIOTime / finalTime);
    double avgTAT = (totTATime / (double)procNum);
    double avgCPUWait = (totCPUWait / (double)procNum);
    double throughput = 100.0 * ((double)procNum / finalTime);

    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", (int)finalTime, cpu_util, io_util, avgTAT, avgCPUWait, throughput);
}
// TESTING FUNCTIONS END

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        cout << "Insufficient Arguments Provided.\nRequired Arguments: ./<filename> -s<scheduler-type> <filename> <randfile>";
        exit(0);
    }

    string inputFileName, randomFileName;
    inputFileName = argv[argc - 2];
    randomFileName = argv[argc - 1];

    Scheduler *schedObj;

    schedObj = parseArgs(argc, argv);
    parseRandomFile(randomFileName);

    vector<Process *> seqProcessVector;
    seqProcessVector = parseProcessFile(inputFileName);

    initializeEventQueue(seqProcessVector);

    Simulation(schedObj);

    if (schedObj->type == "RR" || schedObj->type == "PRIO" || schedObj->type == "PREPRIO")
        cout << schedObj->type << " " << QUANTUM << endl;
    else
        cout << schedObj->type << endl;
    printFinalProcessArray(seqProcessVector);
    printSummary(seqProcessVector.size());
}