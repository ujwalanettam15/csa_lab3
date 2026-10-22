/*
Cache Simulator
Level one L1 and level two L2 cache parameters are read from file (block size, line per set and set per cache).
The 32 bit address is divided into tag bits (t), set index bits (s) and block offset bits (b)
s = log2(#sets)   b = log2(block size in bytes)  t=32-s-b
32 bit address (MSB -> LSB): TAG || SET || OFFSET

Tag Bits   : the tag field along with the valid bit is used to determine whether the block in the cache is valid or not.
Index Bits : the set index field is used to determine which set in the cache the block is stored in.
Offset Bits: the offset field is used to determine which byte in the block is being accessed.
*/

#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>
using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss
#define NOWRITEMEM 5 // no write to memory
#define WRITEMEM 6   // write to memory

struct config
{
    int L1blocksize;
    int L1setsize;
    int L1size;
    int L2blocksize;
    int L2setsize;
    int L2size;
};

/*********************************** ↓↓↓ Todo: Implement by you ↓↓↓ ******************************************/
// You need to define your cache class here, or design your own data structure for L1 and L2 cache

/*
A single cache block:
    - valid bit (is the data in the block valid?)
    - dirty bit (has the data in the block been modified by means of a write?)
    - tag (the tag bits of the address)
    - data (the actual data stored in the block, in our case, we don't need to store the data)
*/
struct CacheBlock
{
    // we don't actually need to allocate space for data, because we only need to simulate the cache action
    // or else it would have looked something like this: vector<number of bytes> Data; 
    bool valid_bit = false;
    bool dirty_bit = false; 
    unsigned int tag = 0;
};

/*
A CacheSet:
    - a vector of CacheBlocks
    - a counter to keep track of which block to evict next
*/
struct set
{
    // tips: 
    // Associativity: eg. resize to 4-ways set associative cache  
    vector<CacheBlock> cb_val;
    int counter = 0;
};

// You can design your own data structure for L1 and L2 cache; just an example here
class cache
{
    // some cache configuration parameters.
    // cache L1 or L2
    // cache c1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size,
    //          cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size);
    vector<set> l1;
    vector<set> l2;

    int l1_way;
    int l1_set;
    int l1_offset;
    int l1_index;

    int l2_way;
    int l2_set;
    int l2_offset;
    int l2_index;
    

public:
    cache(int L1blocksize, int L1_associativity, int L1_size_val,
          int L2blocksize, int L2_associativity, int L2_size_val){
        //calculate the num of blocks each cache can hold 
        int l1_num_blocks = (L1_size_val * 1024) / L1blocksize;
        int l2_num_blocks = (L2_size_val * 1024) / L2blocksize; 

        if(L1_associativity == 0) {
            l1_way = l1_num_blocks;
        }
        else {
            l1_way = L1_associativity;
        }
        if(L2_associativity == 0) {
            l2_way = l2_num_blocks;
        }
        else {
            l2_way = L2_associativity;
        }

        l1_set = l1_num_blocks / l1_way;
        l2_set = l2_num_blocks / l2_way;

        l1_offset = (int)log2(L1blocksize);
        l2_offset = (int)log2(L2blocksize);
        l1_index = (int)log2(l1_set);
        l2_index = (int)log2(l2_set);

        set l1_copy_temp;
        l1_copy_temp.cb_val.resize(l1_way);
        l1.assign(l1_set, l1_copy_temp);

        set l2_copy_temp;
        l2_copy_temp.cb_val.resize(l2_way);
        l2.assign(l2_set, l2_copy_temp);
    }

    int write(unsigned int addr){
        /*
        step 1: select the set in our L1 cache using set index bits

        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> WriteHit!
                                                    -> Dirty Bit High
        step 3: Otherwise? -> WriteMiss!

        return WH or WM
        */
       unsigned int block_add = addr >> l2_offset;
       unsigned int index = block_add % l2_set;
       unsigned int tag = block_add / l2_set;

       for(int way = 0; way < l2_way; way++) {
            if(l2[index].cb_val[way].valid_bit && l2[index].cb_val[way].tag == tag) {
                l2[index].cb_val[way].dirty_bit = true;
                return WH;
            }
    }
        return WM;
    }

    int writeL2(unsigned int addr){
        /*
        step 1: select the set in our L2 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> WriteHit!
                                                 -> Dirty Bit High
        step 3: Otherwise? -> WriteMiss!

        return {WM or WH, WRITEMEM or NOWRITEMEM}
        */
    }

    auto readL1(unsigned int addr){
        /*
        step 1: select the set in our L1 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> ReadHit!
        step 3: Otherwise? -> ReadMiss!
        return RH or RM
        */
       unsigned int index = addr >> l1_offset & ((1u << l1_index) - 1);
       unsigned int tag = addr >> (l1_offset + l1_index);
       set s = l1[index];
       for (int i=0; i < s.cb_val.size(); i++) {
            if (s.cb_val[i].tag == tag) {
                if (s.cb_val[i].valid_bit) {
                    return RH;
                }
            } 
       }
       return RM;  
    }

    auto readL2(unsigned int addr){
        /*
        step 1: select the set in our L2 cache using set index bits
        step 2: iterate through each way in the current set
            - If Matching tag and Valid Bit High -> ReadHit!
                                                 -> copy dirty bit
        step 3: otherwise? -> ReadMiss! -> need to pull data from Main Memory
        step 4: find a place in L1 for our requested data
            - case 1: empty way in L1 -> place requested data
            - case 2: no empty way in L1 -> evict from L1 to L2
                    - case 2.1: empty way in L2 -> place evicted L1 data there
                    - case 2.2: no empty way in L2 -> evict from L2 to memory

        return {RM or RH, WRITEMEM or NOWRITEMEM}
        */
       unsigned int index = addr >> l1_offset & ((1u << l1_index) - 1);
       unsigned int tag = addr >> (l1_offset + l1_index);
       set s = l2[index];
       for (int i=0; i < s.cb_val.size(); i++) {
            if (s.cb_val[i].tag == tag) {
                if (s.cb_val[i].valid_bit) {
                    return RH;
                }
            } 
       }
       
       
    }
};

// Tips: another example:
class CacheSystem
{
    Cache L1;
    Cache L2;
public:
    int L1AcceState, L2AcceState, MemAcceState;
    auto read(auto addr){};
    auto write(auto addr){};
};
class Cache
{
    set * CacheSet;
public:
    auto read_access(auto addr){};
    auto write_access(auto addr){};
    auto check_exist(auto addr){};
    auto evict(auto addr){};
};
/*********************************** ↑↑↑ Todo: Implement by you ↑↑↑ ******************************************/





int main(int argc, char *argv[])
{

    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open(argv[1]);
    while (!cache_params.eof())                   // read config file
    {
        cache_params >> dummyLine;                // L1:
        cache_params >> cacheconfig.L1blocksize;  // L1 Block size
        cache_params >> cacheconfig.L1setsize;    // L1 Associativity
        cache_params >> cacheconfig.L1size;       // L1 Cache Size
        cache_params >> dummyLine;                // L2:
        cache_params >> cacheconfig.L2blocksize;  // L2 Block size
        cache_params >> cacheconfig.L2setsize;    // L2 Associativity
        cache_params >> cacheconfig.L2size;       // L2 Cache Size
    }
    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";
    traces.open(argv[2]);
    tracesout.open(outname.c_str());
    string line;
    string accesstype;     // the Read/Write access type from the memory trace;
    string xaddr;          // the address from the memory trace store in hex;
    unsigned int addr;     // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;




/*********************************** ↓↓↓ Todo: Implement by you ↓↓↓ ******************************************/
    // Implement by you:
    // initialize the hirearch cache system with those configs
    // probably you may define a Cache class for L1 and L2, or any data structure you like
    if (cacheconfig.L1blocksize!=cacheconfig.L2blocksize){
        printf("please test with the same block size\n");
        return 1;
    }
    // cache c1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size,
    //          cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size);

    int L1AcceState = NA; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
    int L2AcceState = NA; // L2 access state variable, can be one of NA, RH, RM, WH, WM;
    int MemAcceState = NOWRITEMEM; // Main Memory access state variable, can be either NA or WH;

    if (traces.is_open() && tracesout.is_open())
    {
        while (getline(traces, line))
        { // read mem access file and access Cache

            istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)){
                break;
            }
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32>(addr);

            // access the L1 and L2 Cache according to the trace;
            if (accesstype.compare("R") == 0)  // a Read request
            {
                // Implement by you:
                //   read access to the L1 Cache,
                //   and then L2 (if required),
                //   update the access state variable;
                //   return: L1AcceState L2AcceState MemAcceState
                
                // For example:
                // L1AcceState = cache.readL1(addr); // read L1
                // if(L1AcceState == RM){
                //     L2AcceState, MemAcceState = cache.readL2(addr); // if L1 read miss, read L2
                // }
                // else{ ... }
            }
            else{ // a Write request
                // Implement by you:
                //   write access to the L1 Cache, or L2 / main MEM,
                //   update the access state variable;
                //   return: L1AcceState L2AcceState

                // For example:
                // L1AcceState = cache.writeL1(addr);
                // if (L1AcceState == WM){
                //     L2AcceState, MemAcceState = cache.writeL2(addr);
                // }
                // else if(){...}
            }
/*********************************** ↑↑↑ Todo: Implement by you ↑↑↑ ******************************************/




            // Grading: don't change the code below.
            // We will print your access state of each cycle to see if your simulator gives the same result as ours.
            tracesout << L1AcceState << " " << L2AcceState << " " << MemAcceState << endl; // Output hit/miss results for L1 and L2 to the output file;
        }
        traces.close();
        tracesout.close();
    }
    else
        cout << "Unable to open trace or traceout file ";

    return 0;
}
