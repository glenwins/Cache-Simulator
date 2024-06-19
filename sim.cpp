#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <string>
#include <iostream>
#include <bits/stdc++.h>
using namespace std;


class cache{
public:
    int counter;
    uint32_t tag;
    uint32_t index;
    uint32_t tag2;
    uint32_t index2;
    bool dirty;
    bool valid;

};

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
*/






int main (int argc, char *argv[]) {

    /*argc = 9;
    argv[0] = "./sim";
    argv[1] = "32";
    argv[2] = "1024";
    argv[3] = "2";
    argv[4] = "12288";
    argv[5] = "6";
    argv[6] = "0";
    argv[7] = "0";
    argv[8] = "C:/Users/glent/CLionProjects/463Project1/gcc_trace.txt";*/

    FILE *fp;            // File pointer.
    char *trace_file;        // This variable holds the trace file name.
    cache_params_t params;    // Look at the sim.h header file for the definition of struct cache_params_t.
    char rw;            // This variable holds the request's type (read or write) obtained from the trace.
    uint32_t addr;        // This variable holds the request's address obtained from the trace.
    // The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

    // Exit with an error if the number of command-line arguments is incorrect.
    if (argc != 9) {
        printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
        exit(EXIT_FAILURE);
    }

    // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
    params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
    params.L1_SIZE = (uint32_t) atoi(argv[2]);
    params.L1_ASSOC = (uint32_t) atoi(argv[3]);
    params.L2_SIZE = (uint32_t) atoi(argv[4]);
    params.L2_ASSOC = (uint32_t) atoi(argv[5]);
    params.PREF_N = (uint32_t) atoi(argv[6]);
    params.PREF_M = (uint32_t) atoi(argv[7]);
    trace_file = argv[8];

    int blockoffsetSize = log2(params.BLOCKSIZE);
    int numSets1 = params.L1_SIZE / (params.BLOCKSIZE * params.L1_ASSOC);

    int numSets2 = 0;
    if (params.L2_SIZE != 0) {
        numSets2 = params.L2_SIZE / (params.BLOCKSIZE * params.L2_ASSOC);
    }

    int indexSize2 = log2(numSets2);
    int tagSize2 = params.BLOCKSIZE - indexSize2 - blockoffsetSize;

    int indexSize1 = log2(numSets1);


    int tagSize1 = params.BLOCKSIZE - blockoffsetSize - indexSize1;
    // Open the trace file for reading.

    fp = fopen(trace_file, "r");

    if (fp == (FILE *) NULL) {
        // Exit with an error if file open failed.
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    // Print simulator configuration.
    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
    printf("L1_SIZE:    %u\n", params.L1_SIZE);
    printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
    printf("L2_SIZE:    %u\n", params.L2_SIZE);
    printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
    printf("PREF_N:     %u\n", params.PREF_N);
    printf("PREF_M:     %u\n", params.PREF_M);
    printf("trace_file: %s\n", trace_file);


    cache l1[numSets1][params.L1_ASSOC];
    //vector<vector<cache>> l2;

    cache l2[numSets2][params.L2_ASSOC];
    int L1reads = 0;
    int L1readMisses = 0;
    int L1writes = 0;
    int L1writeMisses = 0;
    int L1writebacks = 0;
    int L1prefetches = 0;
    float L1missrate = 0;

    int L2readsdemand = 0;
    int L2readMissesdemand = 0;
    int L2readsprefetch = 0;
    int L2readMissesprefetch = 0;
    int L2writes = 0;
    int L2writeMisses = 0;
    int L2writebacks = 0;
    int L2prefetches = 0;
    float L2missrate = 0;
    int memoryTraffic = 0;

    for (int i = 0; i < params.L1_SIZE / (params.BLOCKSIZE * params.L1_ASSOC); i++) {
        for (int j = 0; j < params.L1_ASSOC; j++) {
            l1[i][j].counter = j;
            l1[i][j].dirty = false;
            l1[i][j].tag = 0;
            l1[i][j].tag2 = 0;
            l1[i][j].index = 0;
            l1[i][j].index2 = 0;

        }
    }
    if(params.L2_SIZE!=0) {
        for (int i = 0; i < params.L2_SIZE / (params.BLOCKSIZE * params.L2_ASSOC); i++) {

            for (int j = 0; j < params.L2_ASSOC; j++) {
                l2[i][j].counter = j;
                l2[i][j].dirty = false;
                l2[i][j].tag = 0;
                l2[i][j].tag2 = 0;
                l2[i][j].index = 0;
                l2[i][j].index2 = 0;
            }
        }
    }

    // Read requests from the trace file and echo them back.
    while (fscanf(fp, "%c %x\n", &rw, &addr) ==2) {    // Stay in the loop if fscanf() successfully parsed two tokens as specified.

        uint32_t tagindex = addr >> (blockoffsetSize);
        uint32_t tag1 = addr >> (blockoffsetSize + indexSize1);
        uint32_t tag2 = addr >> (blockoffsetSize + indexSize2);
        uint32_t index1 = tagindex - (tag1 << indexSize1);
        uint32_t index2 = tagindex - (tag2 << indexSize2);

        if (rw == 'r') {

            ++L1reads;
            int temp = 0;
            int temp1 = 0;
            int temp2 = 0;

            //printf("r %x %x %x\n", addr, tag1, index1);
            //THIS IS WHERE L1 READ HIT IS PROCESSED
            for (int i = 0; i < params.L1_ASSOC; i++) {
                if (l1[index1][i].tag == tag1) {
                    temp = 1;
                    for (int j = 0; j < params.L1_ASSOC; j++) {
                        if (l1[index1][j].counter < l1[index1][i].counter) {
                            ++l1[index1][j].counter;
                        }
                    }
                    l1[index1][i].counter = 0;
                    break;
                }
            }
            //L1 READ MISS IS PROCESSED
            if (temp == 0) {
                ++L1readMisses;
                if(params.L2_SIZE!=0) { ++L2readsdemand; }
                for (int i = 0; i < params.L1_ASSOC; i++) {

                    if (l1[index1][i].counter == (params.L1_ASSOC - 1)) {
                        for (int j = 0; j < (params.L1_ASSOC); j++) {
                            if (l1[index1][j].counter < l1[index1][i].counter) {
                                ++l1[index1][j].counter;
                            }
                        }

                        if (l1[index1][i].dirty) {
                            ++L1writebacks;
                            if(params.L2_SIZE!=0) {
                                ++L2writes;
                                for (int k = 0; k < params.L2_ASSOC; k++) {
                                    if (l2[l1[index1][i].index2][k].tag == l1[index1][i].tag2) {
                                        for (int j = 0; j < params.L2_ASSOC; j++) {
                                            if (l2[l1[index1][i].index2][j].counter < l2[l1[index1][i].index2][k].counter) {
                                                ++l2[l1[index1][i].index2][j].counter;
                                            }
                                        }
                                        l2[l1[index1][i].index2][k].counter = 0;
                                        l2[l1[index1][i].index2][k].dirty = true;
                                        //printf("\nL2 write: %x %d\n",l2[index2][i].tag,l2[index2][i].dirty);
                                        temp2 = 1;
                                        break;
                                    }
                                }
                                //L2 WRITE MISS IS BEING PROCESSED HERE
                                if (temp2 == 0) {
                                    ++L2writeMisses;
                                    for (int k = 0; k < params.L2_ASSOC; k++) {
                                        if (l2[index2][k].counter == (params.L2_ASSOC - 1)) {
                                            for (int j = 0; j < (params.L2_ASSOC); j++) {
                                                if (l2[index2][j].counter < l2[index2][k].counter) {
                                                    ++l2[index2][j].counter;
                                                }
                                            }
                                            l2[index2][k].counter = 0;
                                            l2[index2][k].tag = tag2;
                                            l2[index2][k].index = index2;
                                            if (l2[index2][k].dirty) {
                                                ++L2writebacks;
                                            }
                                            l2[index2][k].dirty = true;
                                            break;
                                        }
                                    }

                                }
                            }

                        }
                        if(params.L2_SIZE!=0) {
                            for (int k = 0; k < params.L2_ASSOC; k++) {
                                if (l2[index2][k].tag == tag2) {
                                    for (int j = 0; j < params.L2_ASSOC; j++) {
                                        if (l2[index2][j].counter < l2[index2][k].counter) {
                                            ++l2[index2][j].counter;
                                        }
                                    }
                                    l2[index2][k].counter = 0;

                                    temp1 = 1;
                                    break;
                                }
                            }
                            //L2 READ MISS IS BEING PROCESSED HERE
                            if (temp1 == 0) {
                                ++L2readMissesdemand;
                                for (int k = 0; k < params.L2_ASSOC; k++) {
                                    if (l2[index2][k].counter == (params.L2_ASSOC - 1)) {
                                        for (int j = 0; j < (params.L2_ASSOC); j++) {
                                            if (l2[index2][j].counter < l2[index2][k].counter) {
                                                ++l2[index2][j].counter;
                                            }
                                        }
                                        l2[index2][k].counter = 0;
                                        l2[index2][k].tag = tag2;
                                        l2[index2][k].index = index2;
                                        //printf("\nL2 read miss: %x %d\n",l2[index2][i].tag,l2[index2][i].dirty);
                                        if (l2[index2][k].dirty) {
                                            ++L2writebacks;
                                        }
                                        l2[index2][k].dirty = false;
                                        break;
                                    }
                                }

                            }
                        }
                        l1[index1][i].counter = 0;
                        l1[index1][i].tag = tag1;
                        l1[index1][i].index = index1;
                        l1[index1][i].tag2 = tag2;
                        l1[index1][i].index2 = index2;
                        l1[index1][i].dirty = false;
                        break;
                    }
                }
                //L2 READ HIT




                //L2 READ MISS INSERTED HERE

            }
        }
        else if (rw == 'w') {
            ++L1writes;
            //printf("w %x %x %x\n", addr, tag1, index1);
            int temp = 0;
            int temp1=0;
            int temp2 = 0;
            //L1 WRITE HIT
            for (int i = 0; i < params.L1_ASSOC; i++) {
                if (l1[index1][i].tag == tag1) {
                    temp = 1;
                    for (int j = 0; j < params.L1_ASSOC; j++) {
                        if (l1[index1][j].counter < l1[index1][i].counter) {
                            ++l1[index1][j].counter;
                        }
                    }
                    l1[index1][i].counter = 0;
                    l1[index1][i].dirty = true;
                    break;
                }
            }
            //L1 WRITE MISS
            if (temp == 0) {
                ++L1writeMisses;
                if(params.L2_SIZE!=0) { ++L2readsdemand; }
                for (int w = 0; w < params.L1_ASSOC; w++) {
                    if (l1[index1][w].counter == (params.L1_ASSOC - 1)) {
                        for (int j = 0; j < (params.L1_ASSOC); j++) {
                            if (l1[index1][j].counter < l1[index1][w].counter) {
                                ++l1[index1][j].counter;
                            }
                        }

                        //L2 WRITES
                        if (l1[index1][w].dirty) {
                            ++L1writebacks;
                            if(params.L2_SIZE!=0) {
                                ++L2writes;
                                for (int k = 0; k < params.L2_ASSOC; k++) {
                                    if (l2[l1[index1][w].index2][k].tag == l1[index1][w].tag2) {
                                        for (int j = 0; j < params.L2_ASSOC; j++) {
                                            if (l2[l1[index1][w].index2][j].counter < l2[l1[index1][w].index2][k].counter) {
                                                ++l2[l1[index1][w].index2][j].counter;
                                            }
                                        }
                                        l2[l1[index1][w].index2][k].counter = 0;
                                        l2[l1[index1][w].index2][k].dirty = true;
                                        //printf("\nL2 write: %x %d\n",l2[index2][i].tag,l2[index2][i].dirty);

                                        temp2 = 1;
                                        break;
                                    }
                                }
                                //L2 WRITE MISS IS BEING PROCESSED HERE
                                if (temp2 == 0) {
                                    ++L2writeMisses;
                                    for (int k = 0; k < params.L2_ASSOC; k++) {
                                        if (l2[index2][k].counter == (params.L2_ASSOC - 1)) {
                                            for (int j = 0; j < (params.L2_ASSOC); j++) {
                                                if (l2[index2][j].counter < l2[index2][k].counter) {
                                                    ++l2[index2][j].counter;
                                                }
                                            }
                                            l2[index2][k].counter = 0;
                                            l2[index2][k].tag = tag2;
                                            l2[index2][k].index = index2;
                                            if (l2[index2][k].dirty) {
                                                ++L2writebacks;
                                            }
                                            l2[index2][k].dirty = true;
                                            break;
                                        }
                                    }
                                }
                            }

                        }
                        if(params.L2_SIZE!=0) {
                            for (int i = 0; i < params.L2_ASSOC; i++) {
                                if (l2[index2][i].tag == tag2) {
                                    for (int j = 0; j < params.L2_ASSOC; j++) {
                                        if (l2[index2][j].counter < l2[index2][i].counter) {
                                            ++l2[index2][j].counter;
                                        }
                                    }

                                    l2[index2][i].counter = 0;
                                    temp1 = 1;
                                    break;
                                }
                            }

                            //L2 READ MISS IS BEING PROCESSED HERE
                            if (temp1 == 0) {
                                ++L2readMissesdemand;

                                for (int i = 0; i < params.L2_ASSOC; i++) {
                                    if (l2[index2][i].counter == (params.L2_ASSOC - 1)) {

                                        for (int j = 0; j < (params.L2_ASSOC); j++) {
                                            if (l2[index2][j].counter < l2[index2][i].counter) {
                                                ++l2[index2][j].counter;
                                            }
                                        }
                                        l2[index2][i].counter = 0;
                                        l2[index2][i].tag = tag2;
                                        l2[index2][i].index = index2;
                                        //printf("\nL2 read miss: %x %d\n",l2[index2][i].tag,l2[index2][i].dirty);
                                        if (l2[index2][i].dirty) {
                                            ++L2writebacks;
                                        }
                                        l2[index2][i].dirty = false;
                                        break;
                                    }
                                }

                            }}
                        l1[index1][w].counter = 0;
                        l1[index1][w].tag = tag1;
                        l1[index1][w].index = index1;
                        l1[index1][w].tag2 = tag2;
                        l1[index1][w].index2 = index2;
                        l1[index1][w].dirty = true;
                        break;
                    }
                }


                //L1 WRITING TO LRU IN CACHE

            }
        }
        else {
            printf("Error: Unknown request type %c.\n", rw);
            exit(EXIT_FAILURE);
        }

        ///////////////////////////////////////////////////////
        // Issue the request to the L1 cache instance here.
        ///////////////////////////////////////////////////////
    }
    printf("\n===== L1 contents =====");
    for (int i = 0; i < params.L1_SIZE / (params.BLOCKSIZE * params.L1_ASSOC); i++) {
        cout<<'\n';
        cout<<"set      "<<i<<":   ";
        for (int j = 0; j < params.L1_ASSOC; j++) {

            for (int k = 0; k < params.L1_ASSOC; k++){


                if(l1[i][k].counter==j){
                    printf("%x ", l1[i][k].tag);
                    if(l1[i][k].dirty)
                        cout << "D";
                    break;
                }
            }

        }
    }
    if(params.L2_SIZE!=0){printf("\n\n===== L2 contents =====");}
    if(params.L2_SIZE!=0) {
        for (int i = 0; i < params.L2_SIZE / (params.BLOCKSIZE * params.L2_ASSOC); i++) {
            cout << '\n';
            cout << "set      " << i << ":   ";
            for (int j = 0; j < params.L2_ASSOC; j++) {

                for (int k = 0; k < params.L2_ASSOC; k++) {


                    if (l2[i][k].counter == j) {
                        printf("%x ", l2[i][k].tag);
                        if (l2[i][k].dirty)
                            cout << "D";
                        cout << "   ";
                        break;
                    }
                }

            }
        }
    }
    L1missrate = (float(L1readMisses) + float(L1writeMisses)) / (float(L1writes) + float(L1reads));
    memoryTraffic = L1readMisses + L1writeMisses + L1writebacks + L1prefetches;
    if (params.L2_SIZE != 0) {
        L2missrate = (float(L2readMissesdemand)) / (float(L2readsdemand));
        memoryTraffic=L2readMissesdemand+L2readMissesprefetch+L2writeMisses+L2writebacks+L2prefetches;
    }
    cout<<"\n"<<"\n===== Measurements =====";
    cout << "\na. L1 reads:                   " << L1reads << '\n';
    cout << "b. L1 read misses:             " << L1readMisses << '\n';
    cout << "c. L1 writes:                  " << L1writes << '\n';
    cout << "d. L1 write misses:            " << L1writeMisses << '\n';
    cout << "e. L1 miss rate:               ";
    printf("%.4f", L1missrate);
    cout << '\n';
    cout << "f. L1 writebacks:              " << L1writebacks << '\n';
    cout << "g. L1 prefetches:              " << L1prefetches << '\n';

    cout << "h. L2 reads (demand):          " << L2readsdemand << '\n';
    cout << "i. L2 read misses (demand):    " << L2readMissesdemand << '\n';
    cout << "j. L2 reads (prefetch):        " << L2readsprefetch << '\n';
    cout << "k. L2 read misses (prefetch):  " << L2readMissesprefetch << '\n';
    cout << "l. L2 writes:                  " << L2writes << '\n';
    cout << "m. L2 write misses:            " << L2writeMisses << '\n';
    cout << "n. L2 miss rate:               ";
    printf("%.4f", L2missrate);
    cout << '\n';
    cout << "o. L2 writebacks:              " << L2writebacks << '\n';
    cout << "p. L2 prefetches:              " << L2prefetches << '\n';

    cout << "q. memory traffic:             " << memoryTraffic << '\n';


    return (0);

}