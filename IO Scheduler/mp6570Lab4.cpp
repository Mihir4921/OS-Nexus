// NAME: MIHIR PRAJAPATI
// NETID: mp6570
// OS ASSIGNMENT LAB 4

#include <climits>
#include <iostream>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <queue>
#include <vector>
#include <list>
#include <cmath>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

class IORequest
{
public:
    int iop;
    int arrTime;
    int trackAccess;
    int startTime;
    int endTime;
    int waitTime;

    IORequest(int givenIOP, int givenArrTime, int givenTrackAccess)
    {
        iop = givenIOP;
        arrTime = givenArrTime;
        trackAccess = givenTrackAccess;
        startTime = -1;
        endTime = -1;
        waitTime = -1;
    }
};

char *ALGOSPEC;
vector<IORequest *> allSeqIORequests;
bool outputV = false, outputQ = false, outputF = false;
int totalTracksMoved = 0;
int curTrack = 0;

class IOALGO
{
public:
    bool isForwardSeek;
    deque<IORequest *> requestQueue;
    virtual void addRequest(IORequest *toAddIO) = 0;
    virtual bool isQueueEmpty() = 0;
    virtual IORequest *getNextRequest() = 0;
};

class FIFO : public IOALGO
{
public:
    FIFO()
    {
        isForwardSeek = false;
    }
    void addRequest(IORequest *toAddIO)
    {
        requestQueue.push_back(toAddIO);
    }
    bool isQueueEmpty()
    {
        return requestQueue.empty();
    }
    IORequest *getNextRequest()
    {
        IORequest *toReturn = requestQueue.front();
        requestQueue.pop_front();
        if ((toReturn->trackAccess - curTrack) > 0)
        {
            isForwardSeek = true;
        }
        else
        {
            isForwardSeek = false;
        }
        return toReturn;
    }
};

class SSTF : public IOALGO
{
public:
    SSTF()
    {
        isForwardSeek = false;
    }
    void addRequest(IORequest *toAddIO)
    {
        requestQueue.push_back(toAddIO);
    }
    bool isQueueEmpty()
    {
        return requestQueue.empty();
    }
    IORequest *getNextRequest()
    {
        int minSeek = abs((*requestQueue.begin())->trackAccess - curTrack);
        auto minIt = requestQueue.begin();
        for (auto it = requestQueue.begin(); it != requestQueue.end(); it++)
        {
            if (abs((*it)->trackAccess - curTrack) < minSeek)
            {
                minSeek = abs((*it)->trackAccess - curTrack);
                minIt = it;
            }
        }
        IORequest *toReturn = *minIt;
        requestQueue.erase(minIt);
        if ((toReturn->trackAccess - curTrack) > 0)
        {
            isForwardSeek = true;
        }
        else
        {
            isForwardSeek = false;
        }
        return toReturn;
    }
};

class LOOK : public IOALGO
{
public:
    LOOK()
    {
        isForwardSeek = false;
    }
    void addRequest(IORequest *toAddIO)
    {
        requestQueue.push_back(toAddIO);
    }
    bool isQueueEmpty()
    {
        return requestQueue.empty();
    }
    IORequest *getNextRequest()
    {
        int pos = 0, minPos = -1, minDiff = INT_MAX;
        bool foundInCurrDir = false;
        for (auto it = requestQueue.begin(); it != requestQueue.end(); it++)
        {
            int diff = (*it)->trackAccess - curTrack;

            if (isForwardSeek && diff >= 0)
            {
                foundInCurrDir = true;
                if (diff < minDiff)
                {
                    minDiff = diff;
                    minPos = pos;
                }
            }
            else if (!isForwardSeek && diff <= 0)
            {
                foundInCurrDir = true;
                if (abs(diff) < minDiff)
                {
                    minDiff = abs(diff);
                    minPos = pos;
                }
            }
            ++pos;
        }

        if (!foundInCurrDir)
        {
            isForwardSeek = !isForwardSeek;
            return this->getNextRequest();
        }

        IORequest *returnIO = requestQueue.at(minPos);
        requestQueue.erase(requestQueue.begin() + minPos);
        return returnIO;
    }
};

class CLOOK : public IOALGO
{
public:
    CLOOK()
    {
        isForwardSeek = false;
    }
    void addRequest(IORequest *toAddIO)
    {
        requestQueue.push_back(toAddIO);
    }
    bool isQueueEmpty()
    {
        return requestQueue.empty();
    }
    IORequest *getNextRequest()
    {
        int pos = 0, minPos = -1, minDiff = INT_MAX;
        bool foundInCurrDir = false;
        for (auto it = requestQueue.begin(); it != requestQueue.end(); it++)
        {
            int diff = (*it)->trackAccess - curTrack;

            if (diff >= 0)
            {
                foundInCurrDir = true;
                if (diff < minDiff)
                {
                    minDiff = diff;
                    minPos = pos;
                }
            }
            ++pos;
        }

        if (!foundInCurrDir)
        {
            pos = 0;
            minDiff = INT_MAX;
            isForwardSeek = true;
            for (auto it = requestQueue.begin(); it != requestQueue.end(); it++)
            {
                if ((*it)->trackAccess < minDiff)
                {
                    minDiff = (*it)->trackAccess;
                    minPos = pos;
                }
                ++pos;
            }
        }

        IORequest *returnIO = requestQueue.at(minPos);
        requestQueue.erase(requestQueue.begin() + minPos);
        return returnIO;
    }
};

class FLOOK : public IOALGO
{
public:
    deque<IORequest *> addQueue;
    deque<IORequest *> activeQueue;
    FLOOK()
    {
        isForwardSeek = false;
    }
    void addRequest(IORequest *toAddIO)
    {
        addQueue.push_back(toAddIO);
    }
    bool isQueueEmpty()
    {
        return (activeQueue.empty() && addQueue.empty());
    }
    IORequest *getNextRequest()
    {
        if (activeQueue.empty())
        {
            activeQueue.swap(addQueue);
        }

        int pos = 0, minPos = -1, minDiff = INT_MAX;
        bool foundInCurrDir = false;
        for (auto it = activeQueue.begin(); it != activeQueue.end(); it++)
        {
            int diff = (*it)->trackAccess - curTrack;

            if (isForwardSeek && diff >= 0)
            {
                foundInCurrDir = true;
                if (diff < minDiff)
                {
                    minDiff = diff;
                    minPos = pos;
                }
            }
            else if (!isForwardSeek && diff <= 0)
            {
                foundInCurrDir = true;
                if (abs(diff) < minDiff)
                {
                    minDiff = abs(diff);
                    minPos = pos;
                }
            }
            ++pos;
        }

        if (!foundInCurrDir)
        {
            isForwardSeek = !isForwardSeek;
            return this->getNextRequest();
        }

        IORequest *returnIO = activeQueue.at(minPos);
        activeQueue.erase(activeQueue.begin() + minPos);
        return returnIO;
    }
};

IOALGO *ioAlgoScheduler;

void parseArgs(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "vqfs:")) != -1)
    {
        switch (c)
        {
        case 'v':
            outputV = true;
            break;
        case 'q':
            outputQ = true;
            break;
        case 'f':
            outputF = true;
            break;
        case 's':
            ALGOSPEC = optarg;
            switch (ALGOSPEC[0])
            {
            case 'S':
                ioAlgoScheduler = new SSTF();
                break;
            case 'L':
                ioAlgoScheduler = new LOOK();
                break;
            case 'C':
                ioAlgoScheduler = new CLOOK();
                break;
            case 'F':
                ioAlgoScheduler = new FLOOK();
                break;
            default:
                ioAlgoScheduler = new FIFO();
                break;
            }
            break;
        }
    }
}

void parseInputFile(string inputFName)
{
    ifstream inputFileObj(inputFName);
    string curLine;
    int ioNum = 0;
    while (getline(inputFileObj, curLine))
    {

        if (curLine.find("#") != string::npos)
        {
            continue;
        }
        stringstream currentIORequest(curLine);
        string strIOArrTime, strIOTrackAccess;
        currentIORequest >> strIOArrTime >> strIOTrackAccess;
        int ioArrTime, ioTrackAccess;
        ioArrTime = stoi(strIOArrTime);
        ioTrackAccess = stoi(strIOTrackAccess);

        IORequest *curIOReq = new IORequest(ioNum, ioArrTime, ioTrackAccess);
        allSeqIORequests.push_back(curIOReq);
        // printf("%5d: %5d %5d\n", allSeqIORequests[ioNum]->iop, allSeqIORequests[ioNum]->arrTime, allSeqIORequests[ioNum]->trackAccess);
        // printf("%5d: %5d %5d\n", curIOReq->iop, curIOReq->arrTime, curIOReq->trackAccess);
        // cout << endl;
        // for (int i = 0; i < allSeqIORequests.size(); i++)
        // {
        //     printf("%5d: %5d %5d\n", allSeqIORequests[i]->iop, allSeqIORequests[i]->arrTime, allSeqIORequests[i]->trackAccess);
        // }
        // cout << endl;
        ++ioNum;
    }
}
void testPrintAllReqs()
{
    for (int i = 0; i < allSeqIORequests.size(); i++)
    {
        printf("%5d: %5d %5d\n", allSeqIORequests[i]->iop, allSeqIORequests[i]->arrTime, allSeqIORequests[i]->trackAccess);
    }
}

void printAllStats()
{
    int maxWait = -1;
    double totalWait = 0;
    double totalTAT = 0;
    double IOUtil = 0;
    int numRequests = allSeqIORequests.size();
    int totalTime = -1;

    for (int i = 0; i < allSeqIORequests.size(); i++)
    {
        IORequest ioReq = *allSeqIORequests[i];
        printf("%5d: %5d %5d %5d\n", ioReq.iop, ioReq.arrTime, ioReq.startTime, ioReq.endTime);
        totalTAT += (ioReq.endTime - ioReq.arrTime);
        totalWait += (ioReq.waitTime);
        IOUtil += (ioReq.endTime - ioReq.startTime);
        if (ioReq.waitTime > maxWait)
        {
            maxWait = ioReq.waitTime;
        }
        if (ioReq.endTime > totalTime)
        {
            totalTime = ioReq.endTime;
        }
    }

    IOUtil /= totalTime;
    totalTAT /= numRequests;
    totalWait /= numRequests;

    printf("SUM: %d %d %.4lf %.2lf %.2lf %d\n", totalTime, totalTracksMoved, IOUtil, totalTAT, totalWait, maxWait);
}

void simulation()
{
    int curTime = 0;
    int curIOReq = 0;
    IORequest *activeIOReq = nullptr;
    while (true)
    {
        if ((curIOReq < allSeqIORequests.size()) && (curTime == allSeqIORequests[curIOReq]->arrTime))
        {
            // cout << "here\n";
            ioAlgoScheduler->addRequest(allSeqIORequests[curIOReq]);
            if (outputV)
            {
                // cout << "here\n";
                printf("%5d: %5d add %5d\n", curTime, curIOReq, allSeqIORequests[curIOReq]->trackAccess);
            }
            curIOReq++;
            // cout << endl;
            // testPrintAllReqs();
            // cout << endl;
        }

        if (activeIOReq && (activeIOReq->trackAccess == curTrack))
        {
            activeIOReq->endTime = curTime;
            if (outputV)
            {
                printf("%5d: %5d finish %5d\n", curTime, activeIOReq->iop, activeIOReq->endTime - activeIOReq->startTime);
            }
            activeIOReq = nullptr;
        }

        if (activeIOReq == nullptr)
        {
            if (!ioAlgoScheduler->isQueueEmpty())
            {

                activeIOReq = ioAlgoScheduler->getNextRequest();

                activeIOReq->startTime = curTime;

                activeIOReq->waitTime = curTime - activeIOReq->arrTime;
                // cout << endl;
                // testPrintAllReqs();
                // cout << endl;
                if (outputV)
                {
                    printf("%5d: %5d issue %5d %5d\n", curTime, activeIOReq->iop, activeIOReq->trackAccess, curTrack);
                }
                if ((activeIOReq->trackAccess - curTrack) == 0)
                {
                    continue;
                }
                else if ((activeIOReq->trackAccess - curTrack) > 0)
                {
                    ioAlgoScheduler->isForwardSeek = true;
                }
                else
                {
                    ioAlgoScheduler->isForwardSeek = false;
                }
            }
            else if (curIOReq == allSeqIORequests.size())
            {
                return;
            }
        }

        if (activeIOReq)
        {
            if (ioAlgoScheduler->isForwardSeek)
            {
                curTrack++;
            }
            else
            {
                curTrack--;
            }
            totalTracksMoved++;
            // if (activeIOReq->trackAccess == curTrack)
            // {
            //     continue;
            // }
        }
        // cout << endl;
        // testPrintAllReqs();
        // cout << endl;
        ++curTime;
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "Insufficient Arguments Provided.\nRequired Arguments in any order: ./iosched [ -s<schedalgo> | -v | -q | -f ] <inputfile>";
        exit(0);
    }

    string inputFileName, randomFileName;
    inputFileName = argv[argc - 1];

    parseArgs(argc, argv);

    parseInputFile(inputFileName);

    // testPrintProcesses(allProcesses);
    //  testPrintInstructions(allInstructs);

    simulation();
    // testPrintAllReqs();
    printAllStats();
    if (outputV)
    {
        // cout << "here\n";
        // printPageTable(allProcesses);
    }
    if (outputQ)
    {
        // printFrameTable();
    }
    if (outputF)
    {
        // printSummary(allProcesses);
    }
}
