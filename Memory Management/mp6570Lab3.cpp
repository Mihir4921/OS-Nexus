// NAME: MIHIR PRAJAPATI
// NETID: mp6570
// OS ASSIGNMENT LAB 3

#include <iostream>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <queue>
#include <vector>
#include <list>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_FRAMES 128
#define MAX_VPAGES 64

// all costs (need to see if better way to code this; seems messy)
#define LDSTR_COST 1
#define CTXSWITCH_COST 130
#define PROCEXIT_COST 1230
#define MAP_COST 350
#define UNMAP_COST 410
#define INS_COST 3200
#define OUTS_COST 2750
#define FINS_COST 2350
#define FOUTS_COST 2800
#define ZEROS_COST 150
#define SEGV_COST 440
#define SEGPROT_COST 410
#define TAU 49

using namespace std;

vector<int> randomNums;
int randOffset = 0;
int NUMFRAMES;
char *ALGOSPEC;
bool outputO = false, outputP = false, outputF = false, outputS = false;

struct PTE
{
    unsigned PRESENT : 1;
    unsigned REFERENCED : 1;
    unsigned MODIFIED : 1;
    unsigned WRITE_PROTECT : 1;
    unsigned PAGEDOUT : 1;
    unsigned FRAME_POS : 7;
    unsigned FILEMAPPED : 1;
    unsigned VMA_CHECKED : 1;
    unsigned IN_VMA : 1;
    unsigned MISC : 17;
    // unsigned int ASSOC_PID : 4;
};

struct FTE
{
    int pid;
    int pte;
    int fpos;
    unsigned int age;
    unsigned long long lastUseTime;
    PTE *ptePoint;
};
FTE frame_table[MAX_FRAMES];
queue<int> freeFrameList;

struct INSTRUCTION
{
    char type;
    unsigned long long targetNum;
};

class INSTRUCTION_STORE
{
public:
    vector<INSTRUCTION> instructList;
    unsigned long long curInstruct;

    INSTRUCTION_STORE()
    {
        curInstruct = 0;
    }

    int getInstruction(char *op, unsigned long long *target)
    {
        if (curInstruct == instructList.size())
        {
            return 0;
        }
        *op = instructList[curInstruct].type;
        *target = instructList[curInstruct++].targetNum;
        return 1;
    }
};

unsigned long long INST_COUNT = 0;
unsigned long long CTX_SWITCHES = 0;
unsigned long long PROC_EXITS = 0;
unsigned long long COST = 0;

class VMA
{
public:
    int start_vpage;
    int end_vpage;
    int write_protected;
    int filemapped;

    VMA(int stVpage, int endVpage, int wpBit, int filemapBit)
    {
        start_vpage = stVpage;
        end_vpage = endVpage;
        write_protected = wpBit;
        filemapped = filemapBit;
    }
};

class Process
{
public:
    vector<VMA> procVMAList;
    PTE pageTable[MAX_VPAGES];
    int pid;
    unsigned long long unmaps;
    unsigned long long maps;
    unsigned long long ins;
    unsigned long long outs;
    unsigned long long fins;
    unsigned long long fouts;
    unsigned long long zeros;
    unsigned long long segv;
    unsigned long long segprot;

    Process(int procId)
    {
        pid = procId;
        unmaps = 0;
        maps = 0;
        ins = 0;
        outs = 0;
        fins = 0;
        fouts = 0;
        zeros = 0;
        segv = 0;
        segprot = 0;
    }
};

int getRandom(int remainder)
{
    if (randOffset == randomNums.size())
        randOffset = 0;
    return (randomNums[randOffset++] % remainder);
}

class Pager
{
public:
    int framePoint;
    virtual FTE select_next_victim_frame() = 0;
};

class FIFO : public Pager
{
public:
    FIFO()
    {
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        if (framePoint == NUMFRAMES)
            framePoint = 0;
        FTE victimFrame = frame_table[framePoint];
        frame_table[framePoint].pid = -1;
        framePoint++;
        return victimFrame;
    }
};

class RANDOM : public Pager
{
public:
    RANDOM()
    {
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        framePoint = getRandom(NUMFRAMES);
        FTE victimFrame = frame_table[framePoint];
        frame_table[framePoint].pid = -1;
        return victimFrame;
    }
};

class CLOCK : public Pager
{
public:
    CLOCK()
    {
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        FTE victimFrame;
        while (true)
        {
            if (frame_table[framePoint].ptePoint->REFERENCED)
            {
                frame_table[framePoint].ptePoint->REFERENCED = 0;
                framePoint = (framePoint + 1) % NUMFRAMES;
            }
            else
            {
                victimFrame = frame_table[framePoint];
                break;
            }
        }
        framePoint = (framePoint + 1) % NUMFRAMES;
        return victimFrame;
    }
};

class NRU : public Pager
{
public:
    int lastResetInst;

    NRU()
    {
        lastResetInst = 0;
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        bool resetRef = ((INST_COUNT - lastResetInst) >= 48) ? true : false;
        if (resetRef)
        {
            lastResetInst = INST_COUNT;
        }
        int frameSearchStart = framePoint;
        vector<int> statusClasses(4, -1);
        for (int i = 0; i < NUMFRAMES; i++)
        {
            int frameClass = (2 * frame_table[framePoint].ptePoint->REFERENCED) + (frame_table[framePoint].ptePoint->MODIFIED);
            // printf("PTE %d R M: %d %d\n", frame_table[framePoint].pte, frame_table[framePoint].ptePoint->REFERENCED, frame_table[framePoint].ptePoint->MODIFIED);
            if (statusClasses[frameClass] == -1)
            {
                // printf("Assigned to Class %d FP %d:\nPTE %d R M: %d %d\n", frameClass, framePoint, frame_table[framePoint].pte, frame_table[framePoint].ptePoint->REFERENCED, frame_table[framePoint].ptePoint->MODIFIED);
                statusClasses[frameClass] = framePoint;
            }
            if (resetRef)
            {
                frame_table[framePoint].ptePoint->REFERENCED = 0;
            }
            else if (frameClass == 0)
            {
                break;
            }
            framePoint = (framePoint + 1) % NUMFRAMES;
        }
        FTE victimFrame;
        for (int i = 0; i < 4; i++)
        {
            if (statusClasses[i] != -1)
            {
                // printf("victimFrame %d of class %d\n", frame_table[statusClasses[i]].fpos, statusClasses[i]);
                victimFrame = frame_table[statusClasses[i]];
                framePoint = (statusClasses[i] + 1) % NUMFRAMES;
                break;
            }
        }
        // printf("removedFramepid: %d\n", victimFrame.pid);
        return victimFrame;
    }
};

class AGING : public Pager
{
public:
    AGING()
    {
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        FTE victimFrame;
        for (int i = 0; i < NUMFRAMES; i++)
        {
            frame_table[i].age = frame_table[i].age >> 1;
            if (frame_table[i].ptePoint->REFERENCED)
            {
                frame_table[i].age = (frame_table[i].age | 0x80000000);
                frame_table[i].ptePoint->REFERENCED = 0;
            }
        }

        int trackFrameIndex = framePoint;
        for (int i = 0; i < NUMFRAMES; i++)
        {
            if (frame_table[trackFrameIndex].age < frame_table[framePoint].age)
            {
                framePoint = trackFrameIndex;
            }
            trackFrameIndex = (trackFrameIndex + 1) % NUMFRAMES;
        }
        victimFrame = frame_table[framePoint];
        framePoint = (framePoint + 1) % NUMFRAMES;
        return victimFrame;
    }
};

class WORKSET : public Pager
{
public:
    WORKSET()
    {
        framePoint = 0;
    }
    FTE select_next_victim_frame()
    {
        FTE victimFrame;
        int trackFrameIndex = framePoint;
        for (int i = 0; i < NUMFRAMES; i++)
        {
            if (frame_table[trackFrameIndex].ptePoint->REFERENCED == 1)
            {
                frame_table[trackFrameIndex].ptePoint->REFERENCED = 0;
                frame_table[trackFrameIndex].lastUseTime = INST_COUNT;
            }
            else if ((INST_COUNT - frame_table[trackFrameIndex].lastUseTime) > TAU)
            {
                framePoint = trackFrameIndex;
                victimFrame = frame_table[framePoint];
                framePoint = (framePoint + 1) % NUMFRAMES;
                return victimFrame;
            }
            trackFrameIndex = (trackFrameIndex + 1) % NUMFRAMES;
        }

        trackFrameIndex = framePoint;
        int maxTime = (INST_COUNT - frame_table[framePoint].lastUseTime);
        for (int i = 0; i < NUMFRAMES; i++)
        {
            if ((INST_COUNT - frame_table[trackFrameIndex].lastUseTime) > maxTime)
            {
                maxTime = INST_COUNT - frame_table[trackFrameIndex].lastUseTime;
                framePoint = trackFrameIndex;
            }
            trackFrameIndex = (trackFrameIndex + 1) % NUMFRAMES;
        }
        victimFrame = frame_table[framePoint];
        framePoint = (framePoint + 1) % NUMFRAMES;
        return victimFrame;
    }
};

Pager *pagerAlgo;

// PARSING START
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

void setOutputBools(string outputArg)
{
    // cout << "here\n";
    if (outputArg.find('O') != string::npos)
    {
        outputO = true;
    }
    if (outputArg.find('P') != string::npos)
    {
        outputP = true;
    }
    if (outputArg.find('F') != string::npos)
    {
        outputF = true;
    }
    if (outputArg.find('S') != string::npos)
    {
        outputS = true;
    }
}

void parseArgs(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "f:a:o:")) != -1)
    {
        switch (c)
        {
        case 'f':
            NUMFRAMES = atoi(optarg);
            for (int i = 0; i < NUMFRAMES; i++)
            {
                frame_table[i].pid = -1;
                frame_table[i].pte = -1;
                frame_table[i].fpos = i;
            }
            break;
        case 'a':
            ALGOSPEC = optarg;
            switch (ALGOSPEC[0])
            {
            case 'f':
                pagerAlgo = new FIFO();
                break;
            case 'r':
                pagerAlgo = new RANDOM();
                break;
            case 'c':
                pagerAlgo = new CLOCK();
                break;
            case 'e':
                pagerAlgo = new NRU();
                break;
            case 'a':
                pagerAlgo = new AGING();
                break;
            case 'w':
                pagerAlgo = new WORKSET();
                break;
            }
            break;
        case 'o':
            string outputSpec = optarg;
            setOutputBools(outputSpec);
            break;
        }
    }
}

void setAllProcesses(ifstream *iFObj, vector<Process *> *procList, int pNum)
{
    // After getting total num of processes loop through the num of processes
    // to get details of each process
    string curLine;
    for (int curProcNum = 0; curProcNum < pNum; curProcNum++)
    {
        int procVMANum;
        while (getline(*iFObj, curLine))
        {

            if (curLine.find("#") != string::npos)
            {
                continue;
            }

            // for each process first get the number of VMAs
            procVMANum = stoi(curLine);
            Process *curProc = new Process(curProcNum);
            // then loop through each VMA details and store them
            for (int curVMANum = 0; curVMANum < procVMANum; curVMANum++)
            {
                // int startPNum, endPNum, wp, fm;
                int startPNum, endPNum, wp, fm;
                while (getline(*iFObj, curLine))
                {

                    if (curLine.find("#") != string::npos)
                    {
                        continue;
                    }

                    // to read the 4 attributes of VMA
                    stringstream VMADef(curLine);
                    string token;

                    VMADef >> token;
                    startPNum = stoi(token);
                    VMADef >> token;
                    endPNum = stoi(token);
                    VMADef >> token;
                    wp = stoi(token);
                    VMADef >> token;
                    fm = stoi(token);

                    // now to initialize a VMA object and insert it into the process
                    VMA curVMA = VMA(startPNum, endPNum, wp, fm);
                    curProc->procVMAList.push_back(curVMA);
                    break;
                }
            }

            // !!WARNING!! Initialzie PAGETABLE (NOT SURE IF SHOULD BE DONE)
            for (int pteNum = 0; pteNum < MAX_VPAGES; pteNum++)
            {
                // curProc->pageTable[pteNum].ASSOC_PID = 0;
                // curProc->pageTable[pteNum].FILEMAPPED = 0;
                curProc->pageTable[pteNum].FRAME_POS = 0;
                curProc->pageTable[pteNum].IN_VMA = 0;
                curProc->pageTable[pteNum].MODIFIED = 0;
                curProc->pageTable[pteNum].PAGEDOUT = 0;
                curProc->pageTable[pteNum].PRESENT = 0;
                curProc->pageTable[pteNum].REFERENCED = 0;
                curProc->pageTable[pteNum].VMA_CHECKED = 0;
                // curProc->pageTable[pteNum].WRITE_PROTECT = 0;
            }

            procList->push_back(curProc);

            break;
        }
    }
}

void readAllInstructions(ifstream *iFObj, INSTRUCTION_STORE *instList)
{
    string curLine;
    while (getline(*iFObj, curLine))
    {
        if (curLine.find("#") != string::npos)
        {
            continue;
        }
        stringstream instruct(curLine);
        string token;
        INSTRUCTION curInstruct;
        instruct >> token;
        curInstruct.type = token[0];
        instruct >> token;
        curInstruct.targetNum = stoi(token);
        instList->instructList.push_back(curInstruct);
    }
}

void parseInputFile(string inputFName, vector<Process *> *procList, INSTRUCTION_STORE *allInst)
{
    ifstream inputFileObj(inputFName);
    string curLine;
    int procNum;
    while (getline(inputFileObj, curLine))
    {

        if (curLine.find("#") != string::npos)
        {
            continue;
        }
        procNum = stoi(curLine);
        break;
    }

    setAllProcesses(&inputFileObj, procList, procNum);
    readAllInstructions(&inputFileObj, allInst);
}

void checkAndSetVMA(Process *toSetProc, int checkVPage)
{
    PTE *toSetPTE = &(toSetProc->pageTable[checkVPage]);
    for (auto vma : toSetProc->procVMAList)
    {
        if (vma.start_vpage <= checkVPage && vma.end_vpage >= checkVPage)
        {
            toSetPTE->IN_VMA = 1;
            toSetPTE->FILEMAPPED = vma.filemapped;
            toSetPTE->WRITE_PROTECT = vma.write_protected;
        }
    }
    toSetPTE->VMA_CHECKED = 0;
}

int allocateFrame(int procIdAlloc, int vPageAlloc)
{
    if (!freeFrameList.empty())
    {
        int freeFrame = freeFrameList.front();
        freeFrameList.pop();
        frame_table[freeFrame].pid = procIdAlloc;
        frame_table[freeFrame].pte = vPageAlloc;
        return freeFrame;
    }
    for (int phyFrame = 0; phyFrame < NUMFRAMES; phyFrame++)
    {
        if (frame_table[phyFrame].pid == -1)
        {
            frame_table[phyFrame].pid = procIdAlloc;
            frame_table[phyFrame].pte = vPageAlloc;
            return phyFrame;
        }
    }
    return -1;
}

int pageFaultHandler(vector<Process *> *allProcList, int vPage, Process *theProc)
{
    PTE *thePTE = &(theProc->pageTable[vPage]);
    if (!thePTE->VMA_CHECKED)
    {
        checkAndSetVMA(theProc, vPage);
    }
    if (thePTE->IN_VMA)
    {
        int frameAllocPos = allocateFrame(theProc->pid, vPage);
        if (frameAllocPos == -1)
        {
            FTE removedFrame = pagerAlgo->select_next_victim_frame();
            COST += UNMAP_COST;

            Process *removedProc = (*allProcList)[removedFrame.pid];

            PTE *removedPTE = &(removedProc->pageTable[removedFrame.pte]);

            removedProc->unmaps++;
            if (outputO)
            {
                printf(" UNMAP %d:%d\n", removedFrame.pid, removedFrame.pte);
            }

            removedPTE->PRESENT = 0;
            if (removedPTE->MODIFIED)
            {

                if (removedPTE->FILEMAPPED)
                {
                    COST += FOUTS_COST;
                    removedProc->fouts++;
                    if (outputO)
                    {
                        printf(" FOUT\n");
                    }
                }
                else
                {
                    COST += OUTS_COST;
                    removedProc->outs++;
                    removedPTE->PAGEDOUT = 1;
                    if (outputO)
                    {
                        printf(" OUT\n");
                    }
                }
                removedPTE->MODIFIED = 0;
            }
            frameAllocPos = removedFrame.fpos;
            frame_table[frameAllocPos].pid = theProc->pid;
            frame_table[frameAllocPos].pte = vPage;

            // TO CHECK IF MODIFIED TO BE SET TO 0 OR NOT
        }

        // !! TODO # IDEA: frameAllocPos gets changed in the paging algo block above so that no if condition required.
        thePTE->PRESENT = 1;
        thePTE->FRAME_POS = frameAllocPos;
        // thePTE->REFERENCED = 1; // NEED TO MAKE SURE IF TO DO OR NOT
        frame_table[frameAllocPos].ptePoint = thePTE;

        if (thePTE->PAGEDOUT == 1)
        {
            COST += INS_COST;
            theProc->ins++;
            if (outputO)
            {
                printf(" IN\n");
            }
        }
        else if (thePTE->FILEMAPPED == 1)
        {
            COST += FINS_COST;
            theProc->fins++;
            if (outputO)
            {
                printf(" FIN\n");
            }
        }
        else
        {
            COST += ZEROS_COST;
            theProc->zeros++;
            if (outputO)
            {
                printf(" ZERO\n");
            }
        }
        if (outputO)
        {
            printf(" MAP %d\n", frameAllocPos);
        }
        frame_table[frameAllocPos].age = 0x00000000;
        frame_table[frameAllocPos].lastUseTime = INST_COUNT; // !! CHECK IF INST_COUNT OR INST_COUNT - 1
        COST += MAP_COST;
        theProc->maps++;
    }
    else
    {
        // !! SEGV CASE: CHECK TO HANDLE
        COST += SEGV_COST;
        theProc->segv++;
        if (outputO)
        {
            printf(" SEGV\n");
        }
        return 1;
    }
    return 0;
}

void simulation(vector<Process *> *procList, INSTRUCTION_STORE *allInsts)
{
    Process *curProc;
    char operation;
    unsigned long long opTarget;
    while (allInsts->getInstruction(&operation, &opTarget))
    {
        if (outputO)
        {
            printf("%llu: ==> %c %llu\n", INST_COUNT, operation, opTarget);
        }
        INST_COUNT++;
        if (operation == 'c')
        {
            curProc = (*procList)[opTarget];
            COST += CTXSWITCH_COST;
            CTX_SWITCHES++;
            continue;
        }
        if (operation == 'e')
        {
            curProc = (*procList)[opTarget];
            printf("EXIT current process %d\n", curProc->pid);
            for (int i = 0; i < MAX_VPAGES; i++)
            {
                PTE *curPTE = &curProc->pageTable[i];
                if (curPTE->PRESENT)
                {
                    COST += UNMAP_COST;
                    curProc->unmaps++;
                    if (outputO)
                    {
                        printf(" UNMAP %d:%d\n", curProc->pid, i);
                    }

                    curPTE->PRESENT = 0;
                    if (curPTE->MODIFIED)
                    {

                        if (curPTE->FILEMAPPED)
                        {
                            COST += FOUTS_COST;
                            curProc->fouts++;
                            if (outputO)
                            {
                                printf(" FOUT\n");
                            }
                        }
                    }

                    int nowFreeFrame = curPTE->FRAME_POS;
                    freeFrameList.push(nowFreeFrame);
                    frame_table[nowFreeFrame].pid = -1;
                    frame_table[nowFreeFrame].age = 0x00000000;
                }
                curPTE->PAGEDOUT = 0;
            }
            PROC_EXITS++;
            COST += PROCEXIT_COST;
            continue;
        }
        // IS THIS THE CORRECT POSITION FOR THIS
        COST += LDSTR_COST;
        PTE *curPTE = &(curProc->pageTable[opTarget]);
        if (curPTE->PRESENT == 0)
        {

            if (pageFaultHandler(procList, opTarget, curProc))
            {
                // CASE OF SEGV
                continue;
            }
        }

        curPTE->REFERENCED = 1;
        if (operation == 'w')
        {
            if (!curPTE->WRITE_PROTECT)
            {
                curPTE->MODIFIED = 1;
            }
            else
            {
                curProc->segprot++;
                COST += SEGPROT_COST;
                if (outputO)
                {
                    printf(" SEGPROT\n");
                }
            }
        }
    }
}

void testPrintProcesses(vector<Process *> procList)
{
    for (auto curProc : procList)
    {
        cout << curProc->pid << endl;
        cout << "VMAs:\n";
        for (auto vma : curProc->procVMAList)
        {
            printf("%d %d %d %d\n", vma.start_vpage, vma.end_vpage, vma.write_protected, vma.filemapped);
        }
    }
    cout << endl;
}

void testPrintInstructions(INSTRUCTION_STORE allInsts)
{
    for (auto curInst : allInsts.instructList)
    {
        printf("%c %llu\n", curInst.type, curInst.targetNum);
    }
    cout << endl;
}

void printPageTable(vector<Process *> procList)
{
    for (int curProcNum = 0; curProcNum < procList.size(); curProcNum++)
    {
        Process *curProc = procList[curProcNum];
        printf("PT[%d]:", curProcNum);
        for (int pageNum = 0; pageNum < MAX_VPAGES; pageNum++)
        {
            PTE curPTE = curProc->pageTable[pageNum];
            if (curPTE.PRESENT == 0)
            {
                if (curPTE.PAGEDOUT)
                {
                    printf(" #");
                }
                else
                {
                    printf(" *");
                }
            }
            else
            {
                printf(" %d:", pageNum);
                if (curPTE.REFERENCED)
                    printf("R");
                else
                    printf("-");
                if (curPTE.MODIFIED)
                    printf("M");
                else
                    printf("-");
                if (curPTE.PAGEDOUT)
                    printf("S");
                else
                    printf("-");
            }
        }
        printf("\n");
    }
}

void printFrameTable()
{
    printf("FT:");
    for (int i = 0; i < NUMFRAMES; i++)
    {
        if (frame_table[i].pid != -1)
            printf(" %d:%d", frame_table[i].pid, frame_table[i].pte);
        else
            printf(" *");
    }
    printf("\n");
}

void printSummary(vector<Process *> procList)
{
    for (int curProcNum = 0; curProcNum < procList.size(); curProcNum++)
    {
        Process *curProc = procList[curProcNum];
        printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu\n", curProc->pid, curProc->unmaps, curProc->maps, curProc->ins, curProc->outs, curProc->fins, curProc->fouts, curProc->zeros, curProc->segv, curProc->segprot);
    }
    printf("TOTALCOST %llu %llu %llu %llu %lu\n", INST_COUNT, CTX_SWITCHES, PROC_EXITS, COST, sizeof(PTE));
}

int main(int argc, char **argv)
{
    if (argc < 6)
    {
        cout << "Insufficient Arguments Provided.\nRequired Arguments in any order: ./mmu -f<num_frames> -a<algo> [-o<options>] inputfile randomfile";
        exit(0);
    }

    string inputFileName, randomFileName;
    inputFileName = argv[argc - 2];
    randomFileName = argv[argc - 1];

    parseArgs(argc, argv);

    vector<Process *> allProcesses;
    INSTRUCTION_STORE allInstructs = INSTRUCTION_STORE();

    parseInputFile(inputFileName, &allProcesses, &allInstructs);
    parseRandomFile(randomFileName);

    // testPrintProcesses(allProcesses);
    //  testPrintInstructions(allInstructs);

    simulation(&allProcesses, &allInstructs);

    if (outputP)
    {
        // cout << "here\n";
        printPageTable(allProcesses);
    }
    if (outputF)
    {
        printFrameTable();
    }
    if (outputS)
    {
        printSummary(allProcesses);
    }
}
