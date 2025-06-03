// NAME: MIHIR PRAJAPATI
// NETID: mp6570
// OS ASSIGNMENT LAB 1

#include <iostream>
#include <cctype>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <map>

#define MACHINE_SIZE 512
#define MAX_LINE_LEN 4096
#define MAX_INT_PERMIT 1073741823

using namespace std;

unordered_map<string, int> symbolTable;
unordered_map<int, int> moduleBaseTable;
int parseErrorEOF;
int lineLen;

void __parseerror(int errcode, char *tokRef, int &lNum, char *nextLine)
{
    const static char *errstr[] = {
        "NUM_EXPECTED",           // Number expect, anything >= 2^30 is not a number either
        "SYM_EXPECTED",           // Symbol Expected
        "MARIE_EXPECTED",         // Addressing Expected which is M/A/R/I/E
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };
    printf("Parse Error line %d offset %ld: %s\n", lNum, (tokRef - nextLine) + 1, errstr[errcode]);
    exit(1);
}

void __ruleViolation(int ruleCode)
{
    const static char *ruleStr[] = {
        "", // Number expect, anything >= 2^30 is not a number either
        "", // Symbol Expected
        "", // Addressing Expected which is M/A/R/I/E
        "", // Symbol Name is too long
        "", // > 16
        "", // > 16
        "Error: External operand exceeds length of uselist; treated as relative=0\n",
        "",
        "Error: Absolute address exceeds machine size; zero used\n",
        "Error: Relative address exceeds module size; relative zero used\n",
        "Error: Illegal immediate operand; treated as 999\n",
        "Error: Illegal opcode; treated as 9999\n",
        "Error: Illegal module operand ; treated as module=0\n",
    };
    printf("%s", ruleStr[ruleCode]);
}

char *readInt(ifstream &fileTrack, char *tokRef, int &lNum, char *nextLine)
{
    // cout << "Inside readInt function." << endl;
    if (tokRef != NULL)
    {
        tokRef = strtok(NULL, " \t");
    }
    if (tokRef == NULL)
    {
        while (tokRef == NULL && fileTrack.getline(nextLine, MAX_LINE_LEN))
        {
            lineLen = strlen(nextLine);
            tokRef = strtok(nextLine, " \t");
            lNum++;
            if (fileTrack.eof())
            {
                parseErrorEOF = lineLen;
            }
            else
            {
                parseErrorEOF = lineLen + 1;
            }
        };

        if (tokRef == NULL && fileTrack.peek() == EOF)
        {
            printf("Parse Error line %d offset %d: %s\n", lNum, parseErrorEOF, "NUM_EXPECTED");
            exit(1);
        }
    }
    for (int i = 0; i < strlen(tokRef); i++)
    {
        if (!isdigit(tokRef[i]))
        {
            __parseerror(0, tokRef, lNum, nextLine);
        }
    }

    int tokenInt = atoi(tokRef);
    if (tokenInt < 0 || tokenInt > MAX_INT_PERMIT)
    {
        __parseerror(0, tokRef, lNum, nextLine);
    }

    // printf("token=<%s> position=%d:%ld\n", tokRef, lNum, (tokRef - nextLine) + 1);
    // cout << "Exiting readInt function." << endl<< endl;
    return tokRef;
}

char *readSymbol(ifstream &fileTrack, char *tokRef, int &lNum, char *nextLine)
{
    // cout << "Inside readSymbol function." << endl;

    if (tokRef != NULL)
    {
        tokRef = strtok(NULL, " \t");
    }
    if (tokRef == NULL)
    {
        while (tokRef == NULL && fileTrack.getline(nextLine, MAX_LINE_LEN))
        {
            lineLen = strlen(nextLine);
            tokRef = strtok(nextLine, " \t");
            lNum++;
            if (fileTrack.eof())
            {
                parseErrorEOF = lineLen;
            }
            else
            {
                parseErrorEOF = lineLen + 1;
            }
        };

        if (tokRef == NULL && fileTrack.peek() == EOF)
        {
            printf("Parse Error line %d offset %d: %s\n", lNum, parseErrorEOF, "SYM_EXPECTED");
            exit(1);
        }
    }
    if (strlen(tokRef) > 16)
    {
        __parseerror(3, tokRef, lNum, nextLine);
    }
    if (!isalpha(tokRef[0]))
    {
        __parseerror(1, tokRef, lNum, nextLine);
    }

    for (int i = 1; i < strlen(tokRef); i++)
    {
        if (!isalnum(tokRef[i]))
        {
            __parseerror(1, tokRef, lNum, nextLine);
        }
    }

    // printf("token=<%s> position=%d:%ld\n", tokRef, lNum, (tokRef - nextLine) + 1);
    // cout << "Exiting readSymbol function." << endl<< endl;
    return tokRef;
}

char *readMARIE(ifstream &fileTrack, char *tokRef, int &lNum, char *nextLine)
{
    // cout << "Inside readMARIE function." << endl;
    if (tokRef != NULL)
    {
        tokRef = strtok(NULL, " \t");
    }
    if (tokRef == NULL)
    {
        while (tokRef == NULL && fileTrack.getline(nextLine, MAX_LINE_LEN))
        {
            lineLen = strlen(nextLine);
            tokRef = strtok(nextLine, " \t");
            lNum++;
            if (fileTrack.eof())
            {
                parseErrorEOF = lineLen;
            }
            else
            {
                parseErrorEOF = lineLen + 1;
            }
        };

        if (tokRef == NULL && fileTrack.peek() == EOF)
        {
            printf("Parse Error line %d offset %d: %s\n", lNum, parseErrorEOF, "MARIE_EXPECTED");
            exit(1);
        }
    }
    if ((strcmp(tokRef, "M") == 0) || (strcmp(tokRef, "A") == 0) || (strcmp(tokRef, "R") == 0) || (strcmp(tokRef, "I") == 0) || (strcmp(tokRef, "E") == 0))
    {
        // printf("token=<%s> position=%d:%ld\n", tokRef, lNum, (tokRef - nextLine) + 1);
        // cout << "Exiting readMARIE function." << endl<< endl;
        return tokRef;
    }
    else
    {
        __parseerror(2, tokRef, lNum, nextLine);
    }
    return tokRef;
}

int addSymbol(string symbolFound, int valGiven)
{
    if (symbolTable.find(symbolFound) == symbolTable.end())
    {
        symbolTable[symbolFound] = valGiven;
        return 0;
    }
    else
    {
        return 1;
    }
    return 0;
}

void pass1(ifstream &mainFileObj)
{
    char line[MAX_LINE_LEN];
    int lineNum = 0;
    char *token = NULL;
    int curModuleNum = 0, instructsSoFar = 0, curModuleBaseAddr = 0;
    map<string, int> redefErrorTrack;
    vector<string> seqSymbolStore;
    while (mainFileObj.peek() != EOF)
    {
        // deflist stores the symbols encountered, along with a flag number indicating whether it is a redefinition or not.
        vector<pair<string, int>> deflist;
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int defcount = atoi(token);

        if (defcount < 0)
            exit(2); // eof reached
        if (defcount > 16)
        {
            __parseerror(4, token, lineNum, line);
        }

        // READ SYMBOLS
        for (int i = 0; i < defcount; i++)
        {
            token = readSymbol(mainFileObj, token, lineNum, line);
            string symbol = token;
            token = readInt(mainFileObj, token, lineNum, line);
            int symbolVal = atoi(token);
            // repeatFlag indicates redefinition of symbol.
            int repeatFlag = addSymbol(symbol, symbolVal);
            deflist.push_back(make_pair(symbol, repeatFlag));
            if (repeatFlag)
            {
                redefErrorTrack[symbol] = 1;
            }
            else
            {
                seqSymbolStore.push_back(symbol);
            }
        }

        // USELIST
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int usecount = atoi(token);
        if (usecount > 16)
        {
            __parseerror(5, token, lineNum, line);
        }

        for (int i = 0; i < usecount; i++)
        {
            token = readSymbol(mainFileObj, token, lineNum, line);
            // we don’t do anything here this would change in pass2
        }

        // READ PROGRAM TEXT
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int instcount = atoi(token);

        // AFTER GETTING INSTCOUNT, CHECK IF NUM OF INSTRUCTS GO LARGER THAN MACHINE SIZE (512)
        instructsSoFar += instcount;
        if (instructsSoFar > MACHINE_SIZE)
        {
            __parseerror(6, token, lineNum, line);
        }
        for (int i = 0; i < instcount; i++)
        {
            token = readMARIE(mainFileObj, token, lineNum, line);
            token = readInt(mainFileObj, token, lineNum, line);
        }

        // WARNING PRINT OF REDEFINITION AND OUT OF BOUNDS SYMBOL ADDRESS
        for (int i = 0; i < defcount; i++)
        {
            if (deflist[i].second)
            {
                // NEED TO SEE IF, IN THE CASE OF REDEFINITION OUT OF BOUNDS ADDR NEEDS TO BE CHECKED.
                printf("Warning: Module %d: %s redefinition ignored\n", curModuleNum, deflist[i].first.c_str());
                continue;
            }
            int symbolRelAddr = symbolTable[deflist[i].first];
            // cout << symbolRelAddr << " " << instcount << endl;
            //  NEED TO SET ABSOLUTE ADDR OF SYMBOL AT THIS PART AND IN THE IF CONDITION BELOW
            int symbolAbsAddr = curModuleBaseAddr + symbolRelAddr;
            if (symbolRelAddr >= instcount)
            {
                printf("Warning: Module %d: %s=%d valid=[0..%d] assume zero relative\n", curModuleNum, deflist[i].first.c_str(), symbolRelAddr, instcount - 1);
                symbolAbsAddr = curModuleBaseAddr;
            }
            symbolTable[deflist[i].first] = symbolAbsAddr;
        }
        // cout << endl;

        // update Module Base Table
        moduleBaseTable[curModuleNum] = curModuleBaseAddr;
        curModuleBaseAddr += instcount;
        curModuleNum++;
    }

    // PRINT SYMBOL TABLE
    cout << "Symbol Table" << endl;
    int uniqSymbols = seqSymbolStore.size();
    for (int i = 0; i < uniqSymbols; i++)
    {
        cout << seqSymbolStore[i] << "=" << symbolTable[seqSymbolStore[i]];
        if (redefErrorTrack[seqSymbolStore[i]])
        {
            printf(" Error: This variable is multiple times defined; first value used");
        }
        cout << endl;
    }
}

void pass2(ifstream &mainFileObj)
{
    char line[MAX_LINE_LEN];
    int lineNum = 0;
    char *token = NULL;
    int curModuleNum = 0, instructsTillNow = 0, curModuleBaseAddr = 0;
    map<string, int> redefErrorTrack;
    vector<string> seqSymbolStore;
    vector<string> uniqSeqSymbolStore;
    map<int, int> memoryErrTable;
    map<int, int> memoryMap;
    map<string, int> symbolUseTrack;
    map<string, int> symbolModuleMap;
    for (auto it = symbolTable.begin(); it != symbolTable.end(); ++it)
    {
        symbolUseTrack[it->first] = 0;
    }
    while (mainFileObj.peek() != EOF)
    {
        // deflist stores the symbols encountered, along with a flag number indicating whether it is a redefinition or not.
        vector<pair<string, int>> deflist;
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int defcount = atoi(token);

        // READ SYMBOLS
        for (int i = 0; i < defcount; i++)
        {
            token = readSymbol(mainFileObj, token, lineNum, line);
            string symbol = token;
            seqSymbolStore.push_back(symbol);
            if (symbolModuleMap.find(symbol) == symbolModuleMap.end())
            {
                symbolModuleMap[symbol] = curModuleNum;
                uniqSeqSymbolStore.push_back(symbol);
            }

            token = readInt(mainFileObj, token, lineNum, line);
            int symbolVal = atoi(token);
        }

        // USELIST
        vector<string> uselist;
        vector<int> uselistUseCheck;
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int usecount = atoi(token);
        for (int i = 0; i < usecount; i++)
        {
            token = readSymbol(mainFileObj, token, lineNum, line);
            string symbol = token;
            uselist.push_back(symbol);
            uselistUseCheck.push_back(0);
            // we don’t do anything here this would change in pass2
        }

        // READ PROGRAM TEXT
        token = readInt(mainFileObj, token, lineNum, line); // return negative to indicate no more tokens
        int instcount = atoi(token);

        // AFTER GETTING INSTCOUNT, CHECK IF NUM OF INSTRUCTS GO LARGER THAN MACHINE SIZE (512)

        for (int i = 0; i < instcount; i++)
        {
            token = readMARIE(mainFileObj, token, lineNum, line);
            string addrmode = token;
            token = readInt(mainFileObj, token, lineNum, line);
            int instruct = atoi(token);
            int opcode = instruct / 1000;
            int operand = instruct % 1000;
            if (opcode > 9)
            {
                printf("%03d: 9999 ", instructsTillNow);
                __ruleViolation(11);
            }
            else if (!strcmp(addrmode.c_str(), "M"))
            {
                if (moduleBaseTable.find(operand) != moduleBaseTable.end())
                {
                    printf("%03d: %d%03d\n", instructsTillNow, opcode, moduleBaseTable[operand]);
                }
                else
                {
                    printf("%03d: %d%03d ", instructsTillNow, opcode, 0);
                    __ruleViolation(12);
                }
            }
            else if (!strcmp(addrmode.c_str(), "A"))
            {
                if (operand >= MACHINE_SIZE)
                {
                    printf("%03d: %d%03d ", instructsTillNow, opcode, 0);
                    __ruleViolation(8);
                }
                else
                {
                    printf("%03d: %d%03d\n", instructsTillNow, opcode, operand);
                }
            }
            else if (!strcmp(addrmode.c_str(), "R"))
            {
                if (operand < instcount)
                {
                    int absAddrforR = moduleBaseTable[curModuleNum] + operand;
                    if (absAddrforR < MACHINE_SIZE)
                    {
                        printf("%03d: %d%03d\n", instructsTillNow, opcode, moduleBaseTable[curModuleNum] + operand);
                    }
                    else
                    {
                        printf("%03d: %d%03d ", instructsTillNow, opcode, 0);
                        __ruleViolation(8);
                    }
                }
                else
                {
                    printf("%03d: %d%03d ", instructsTillNow, opcode, moduleBaseTable[curModuleNum]);
                    __ruleViolation(9);
                }
            }
            else if (!strcmp(addrmode.c_str(), "I"))
            {
                if (operand < 900)
                {
                    printf("%03d: %d%03d\n", instructsTillNow, opcode, operand);
                }
                else
                {
                    printf("%03d: %d%03d ", instructsTillNow, opcode, 999);
                    __ruleViolation(10);
                }
            }
            else if (!strcmp(addrmode.c_str(), "E"))
            {
                if (operand < usecount)
                {
                    string symbolForE = uselist[operand];
                    uselistUseCheck[operand] = 1;
                    if (symbolTable.find(symbolForE) != symbolTable.end())
                    {
                        printf("%03d: %d%03d\n", instructsTillNow, opcode, symbolTable[symbolForE]);
                        symbolUseTrack[uselist[operand]] = 1;
                    }
                    else
                    {
                        printf("%03d: %d%03d ", instructsTillNow, opcode, 0);
                        printf("Error: %s is not defined; zero used\n", symbolForE.c_str());
                    }
                }
                else
                {
                    printf("%03d: %d%03d ", instructsTillNow, opcode, moduleBaseTable[curModuleNum]);
                    __ruleViolation(6);
                }
            }

            instructsTillNow++;
        }

        for (int i = 0; i < usecount; i++)
        {
            if (!uselistUseCheck[i])
            {
                printf("Warning: Module %d: uselist[%d]=%s was not used\n", curModuleNum, i, uselist[i].c_str());
            }
        }

        curModuleNum++;
    }

    cout << endl;
    int totDefSymbols = uniqSeqSymbolStore.size();
    int totModules = moduleBaseTable.size();
    int moduleTrack = 0;
    for (int i = 0; i < totDefSymbols; i++)
    {
        if (!symbolUseTrack[uniqSeqSymbolStore[i]])
        {
            printf("Warning: Module %d: %s was defined but never used\n", symbolModuleMap[uniqSeqSymbolStore[i]], uniqSeqSymbolStore[i].c_str());
        }
    }
}

int main(int argc, char **argv)
{
    string readFile = "";
    if (argc == 2)
    {
        readFile = argv[1];
    }
    else
    {
        cout << "Only one file is required for input." << endl;
        exit(1);
    }
    ifstream p1FileObj(readFile);

    if (!p1FileObj.is_open())
    {
        cout << "File not opened/found." << endl;
        exit(1);
    }

    pass1(p1FileObj);
    p1FileObj.clear();
    p1FileObj.close();

    ifstream p2FileObj(readFile);

    if (!p2FileObj.is_open())
    {
        cout << "File not opened/found." << endl;
        exit(1);
    }
    cout << "\nMemory Map\n";
    pass2(p2FileObj);
    cout << "\n";
}

/*
void getToken(ifstream &fileTrack)
{
    char lone[2000];
    fileTrack.getline(lone, 2000);
    cout << lone << "\t" << strlen(lone) << endl;
    char *token = strtok(lone, " \t");
    while (token != NULL)
    {
        cout << token << " " << strlen(token) << endl;
        token = strtok(NULL, " \t");
    }
}

void origReadToken(ifstream &fileTrack, char *tokref, int &lnum, char *ln)
{
    cout << "Inside readToken function." << endl;
    while (tokref != NULL)
    {
        // cout << tokref << " " << strlen(tokref) << endl;
        printf("token=<%s> position=%d:%ld\n", tokref, lnum, (tokref - ln) + 1);
        (tokref) = strtok(NULL, " \t");
    }
    cout << "Exiting readToken function." << endl;
    // return tokref;
}

char *readToken(ifstream &fileTrack, char *tokref, int &lnum, char *ln)
{
    cout << "Inside readToken function." << endl;
    while (tokref != NULL)
    {
        // cout << tokref << " " << strlen(tokref) << endl;
        printf("token=<%s> position=%d:%ld\n", tokref, lnum, (tokref - ln) + 1);
        (tokref) = strtok(NULL, " \t");
    }
    cout << "Exiting readToken function." << endl;
    return tokref;
}
*/