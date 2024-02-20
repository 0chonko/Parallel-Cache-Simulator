/*
 * File: assignment1.cpp
 *
 * Framework to implement Task 1 of the Advances in Computer Architecture lab
 * session. This uses the framework library to interface with tracefiles which
 * will drive the read/write requests
 *
 * Author(s): Michiel W. van Tol, Mike Lankamp, Jony Zhang,
 *            Konstantinos Bousias, Simon Polstra
 *
 */

#include <iostream>
#include <iomanip>
#include <systemc>
#include <algorithm>

#include "psa.h"

using namespace std;
using namespace sc_core; // This pollutes namespace, better: only import what you need.

static const size_t MEM_SIZE = 2500;
static const size_t CACHE_SIZE = 32768;
static const size_t LINES_COUNT = 1024;
static const size_t LINE_SIZE = 4; //* sizeof(uint64_t);
static const size_t SET_SIZE = 8;
static const size_t SET_COUNT = 128;
// static const size_t LINE_NUM = 1000;

// whole cache line of 32 bytes can be transferred with a single read or write

// SC_MODULE(Memory) {
//     public:

//     enum Function { FUNC_READ, FUNC_WRITE };
//     enum RetCode { RET_READ_DONE, RET_WRITE_DONE };

//     // signal in
//     sc_in<bool> Port_CLK;
//     sc_in<Function> Port_Func;
//     sc_in<uint64_t> Port_Addr;

//     // signal out
//     sc_out<RetCode> Port_Done;

//     // signal inout
//     sc_inout_rv<64> Port_Data;

//     SC_CTOR(Memory) {
//         SC_THREAD(execute);
//         sensitive << Port_CLK.pos();
//         dont_initialize();
//         // memory data type (64bit addresses)
//         m_data = new uint64_t[MEM_SIZE];
//     }

//     ~Memory() {
//         delete[] m_data;
//     }

//     // dumps the content of the memory
//     void dump() {
//         for (size_t i = 0; i < MEM_SIZE; i++) {
//             cout << setw(5) << i << ": " << setw(5) << m_data[i];
//             if (i % 8 == 7) {
//                 cout << endl;
//             }
//         }
//     }

//     private:
//     uint64_t *m_data;

//     // Direct mapped-block = 12 mod 8=4
//     // 2-way set associative - set = 12 mod 8/2 = 0
//     // Fully associative-one set of 8 lines so anywhere in cache

//     // convert incoming address to 8-way-set
//     size_t addressModuloOperation(uint64_t address) {
//         return Port_Addr % ((CACHE_SIZE / (sizeof(uint64_t) * 4))  / 8);
//     }

//     void execute() {
//         while (true) {
//             wait(Port_Func.value_changed_event());

//             Function f = Port_Func.read();
//             uint64_t addr = Port_Addr.read();
//             uint64_t data = 0;
//             if (f == FUNC_WRITE) {
//                 // cout << sc_time_stamp() << ": MEM received write" << endl;
//                 data = Port_Data.read().to_uint64();
//             } else {
//                 // cout << sc_time_stamp() << ": MEM received read" << endl;
//             }
//             // cout << sc_time_stamp() << ": MEM address " << addr << endl;

//             // This simulates memory read/write delay
//             wait(100);

//             if (f == FUNC_READ) {
//                 // read at addr and return ret code
//                 Port_Data.write((addr < MEM_SIZE) ? m_data[addr] : 0);
//                 Port_Done.write(RET_READ_DONE);
//                 wait();
//                 Port_Data.write(float_64_bit_wire); // string with 64 "Z"'s
//             } else {
//                 if (addr < MEM_SIZE) {
//                     // update addr
//                     m_data[addr] = data;
//                 }
//                 Port_Done.write(RET_WRITE_DONE);
//             }
//         }
//     }
// };

SC_MODULE(Memory)
{
public:
    enum Function
    {
        FUNC_READ,
        FUNC_WRITE
    };
    enum RetCode
    {
        RET_READ_DONE,
        RET_WRITE_DONE
    };
    struct CacheLine
    {
        uint64_t tag;
        uint32_t data[8]; // 32 byte line
        bool state;
        bool dirty;
    };

    struct CacheSet
    {
        CacheLine lines[8];   // 8-way
        uint8_t agingBits[8]; // 3-bit aging bits for each line, stored as 8-bit integers
    };

    // signal in
    sc_in<bool> Port_CLK;
    sc_in<Function> Port_Func;
    sc_in<uint64_t> Port_Addr;

    // signal out
    sc_out<RetCode> Port_Done;

    // signal inout
    sc_inout_rv<64> Port_Data;

    SC_CTOR(Memory)
    {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
        // memory data type (64bit addresses)
        m_data = new uint64_t[CACHE_SIZE];
        cache = new CacheSet[SET_COUNT];
    }

    ~Memory()
    {
        delete[] m_data;
    }

    // dumps the content of the memory
    void dump()
    {
        for (size_t i = 0; i < MEM_SIZE; i++)
        {
            cout << setw(5) << i << ": " << setw(5) << m_data[i];
            if (i % 8 == 7)
            {
                cout << endl;
            }
        }
    }

    // dumps the content of the cache
    void dump_cache()
    {
        for (size_t i = 0; i < SET_COUNT; i++)
        {
            // print every line in every set in the cache
            for (size_t j = 0; j < 8; j++)
            {
                for (size_t k = 0; k < 8; k++)
                {
                    size_t index = (i * 8 * 8) + (j * 8) + k;
                    if (index < CACHE_SIZE)
                    {
                        cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].data[k];
                        if (index % 8 == 7)
                        {
                            cout << endl;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

private:
    uint64_t *m_data;
    CacheSet *cache;

    // Define the number of bits for the set index and block offset
    const int SET_INDEX_BITS = 7;
    const int BLOCK_OFFSET_BITS = 5;
    const int TAG_BITS = 64 - SET_INDEX_BITS - BLOCK_OFFSET_BITS;
    // Calculate the masks for the set index and block offset
    uint64_t setIndexMask = (1ULL << SET_INDEX_BITS) - 1;
    uint64_t blockOffsetMask = (1ULL << BLOCK_OFFSET_BITS) - 1;
    uint64_t setTagMask = ((1ULL << SET_INDEX_BITS) - 1) << BLOCK_OFFSET_BITS;

    uint64_t tagMask = (1ULL << TAG_BITS) - 1;
    // TODO: check for tag

    void update_aging_bits(uint64_t setIndex, uint64_t blockOffset) // active aging bits are 1-8, empty line has 0
    {
        for (int i = 0; i < 8; i++)
        {
            if (i != blockOffset)
            {
                if (cache[setIndex].agingBits[i] > 1)
                {
                    cache[setIndex].agingBits[i] -= 1;
                }
            }
            else
            {
                cache[setIndex].agingBits[i] = 8; // set as freshest entry
            }
        }
    }

    bool containsZero(uint8_t * agingBits)
    {
        return std::any_of(agingBits, agingBits + 8, [](int i)
                           { return i == 0; });
    }

    // function find oldest which searches for oldest line in the set, which has the smallest aging bit in one line (using std::min_element) and should return the index of that bit
    int find_oldest(uint64_t setIndex)
    {
        return std::distance(cache[setIndex].agingBits, std::min_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8));
    }

    void execute()
    {
        while (true)
        {

            wait(Port_Func.value_changed_event());
            // cout << sc_time_stamp() << "data at 10 is: " << cache->lines[0].state << endl;
            Function f = Port_Func.read();
            uint64_t addr = Port_Addr.read();
            uint64_t data = 0;
            uint32_t ret_data = 0;
            // cout << sc_time_stamp() << "your port data received is " << Port_Addr.read() << endl;

            // Extract the set index and block offset
            uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
            uint64_t blockOffset = addr & blockOffsetMask;
            uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
            cout << sc_time_stamp() << "your set and offset are " << setIndex << " " << blockOffset << endl;

            if (f == FUNC_WRITE)
            {
                if (addr < CACHE_SIZE)
                {
                    data = Port_Data.read().to_uint64();

                    // check if the cache line is valid
                    bool emptyLines = containsZero(cache[setIndex].agingBits);

                    if (emptyLines)
                    {
                        for (int i = 0; i < 8; i++)
                        {
                            if (cache[setIndex].agingBits[i] == 0) // empty line
                            {
                                cout << sc_time_stamp() << "empty line found" << endl;
                                cache[setIndex].lines[i].data[blockOffset] = data;
                                cache[setIndex].lines[i].state = 1;
                                cache[setIndex].lines[i].tag = tag;
                                update_aging_bits(setIndex, i);
                                break;
                            }
                        }
                    }
                    else
                    {
                        // first check if theres matching tag in the set
                        bool tagMatch = false;
                        for (int i = 0; i < 8; i++)
                        {
                            // there could be multiple lines having the same tag so you need to check whether
                            if (cache[setIndex].lines[i].tag == tag) // if there's a hit
                            {
                                cout << sc_time_stamp() << "tag match found" << endl;
                                tagMatch = true;
                                cache[setIndex].lines[i].data[blockOffset] = data;
                                cache[setIndex].lines[i].state = 1;
                                cache[setIndex].lines[i].dirty = 1; // set as dirty
                                update_aging_bits(setIndex, i);
                                break;
                            }
                        }
                        if (!tagMatch) // if there's no hit
                        {
                            cout<<sc_time_stamp()<<"no tag match found"<<endl;
                            // find the oldest line to evict
                            int oldest = find_oldest(setIndex);
                            // evict and perform write-back if necessary
                            if (cache[setIndex].lines[oldest].dirty)
                            {
                                cout<<sc_time_stamp()<<"evicting for write miss"<<endl;
                                wait(100); // simulate writing back to memory and allocate-on-write
                            }
                            else
                            {
                                wait(100); // simulate allocate-on-write with data retrieval from memory
                            }
                            cache[setIndex].lines[oldest].data[blockOffset] = data; // replace the data
                            cache[setIndex].lines[oldest].state = 1;
                            update_aging_bits(setIndex, oldest);
                        }
                    }
                    wait(1);
                }
            }
            else
            {
                // read data from cache
                if (addr < CACHE_SIZE)
                {
                    bool tagMatch = false;
                    for (int i = 0; i < 8; i++)
                    {
                        if (cache[setIndex].lines[i].tag == tag) // if there's a hit
                        {
                            tagMatch = true;
                            ret_data = cache[setIndex].lines[i].data[blockOffset];
                            update_aging_bits(setIndex, i);
                            break;
                        }
                    }
                    if (!tagMatch) // if there's no hit
                    {
                        // find the oldest line to evict
                        int oldest = find_oldest(setIndex);
                        // evict and perform write-back if necessary
                        if (cache[setIndex].lines[oldest].dirty)
                        {
                            cout << sc_time_stamp() << "evicting for read miss" << endl;
                            wait(100); // simulate writing back to memory and loading the block from memory
                            cache[setIndex].lines[oldest].dirty = 0; // set as clean
                        }
                        else
                        {
                            wait(100); // simulate loading the block from memory
                        }
                        ret_data = cache[setIndex].lines[oldest].data[blockOffset]; // replace the data
                        update_aging_bits(setIndex, oldest);
                    }
                    wait(1);
                }
            }

            if (f == FUNC_READ)
            {
                // read at addr and return ret code
                Port_Data.write((addr < CACHE_SIZE) ? ret_data : 0);
                Port_Done.write(RET_READ_DONE);
                wait();
                Port_Data.write(float_64_bit_wire); // string with 64 "Z"'s
            }
            else // writing
            {
                Port_Done.write(RET_WRITE_DONE);
            }

            // wait(100);
        }
    }
};

SC_MODULE(CPU)
{
public:
    sc_in<bool> Port_CLK;
    sc_in<Memory::RetCode> Port_MemDone;
    sc_out<Memory::Function> Port_MemFunc;
    sc_out<uint64_t> Port_MemAddr;
    sc_inout_rv<64> Port_MemData;

    SC_CTOR(CPU)
    {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
    }

private:
    void execute()
    {
        TraceFile::Entry tr_data;
        Memory::Function f;

        // Loop until end of tracefile
        while (!tracefile_ptr->eof())
        {
            // Get the next action for the processor in the trace
            if (!tracefile_ptr->next(0, tr_data))
            {
                cerr << "Error reading trace for CPU" << endl;
                break;
            }

            // To demonstrate the statistic functions, we generate a 50%
            // probability of a 'hit' or 'miss', and call the statistic
            // functions below
            int j = rand() % 2;

            switch (tr_data.type)
            {
            case TraceFile::ENTRY_TYPE_READ:
                f = Memory::FUNC_READ;
                if (j)
                    stats_readhit(0);
                else
                    stats_readmiss(0);
                break;

            case TraceFile::ENTRY_TYPE_WRITE:
                f = Memory::FUNC_WRITE;
                if (j)
                    stats_writehit(0);
                else
                    stats_writemiss(0);
                break;

            case TraceFile::ENTRY_TYPE_NOP:
                break;

            default:
                cerr << "Error, got invalid data from Trace" << endl;
                exit(0);
            }

            if (tr_data.type != TraceFile::ENTRY_TYPE_NOP)
            {
                Port_MemAddr.write(tr_data.addr);
                Port_MemFunc.write(f);

                if (f == Memory::FUNC_WRITE)
                {
                    cout << sc_time_stamp() << ": CPU sends write" << endl;

                    // We write the address as the data value.
                    Port_MemData.write(tr_data.addr);
                    wait();
                    Port_MemData.write(float_64_bit_wire); // 64 "Z"'s
                }
                else
                {
                    cout << sc_time_stamp() << ": CPU sends read" << endl;
                }

                wait(Port_MemDone.value_changed_event());

                if (f == Memory::FUNC_READ)
                {
                    cout << sc_time_stamp()
                         << ": CPU reads: " << Port_MemData.read() << endl;
                }
            }
            else
            {
                cout << sc_time_stamp() << ": CPU executes NOP" << endl;
            }
            // Advance one cycle in simulated time
            wait();
        }

        // Finished the Tracefile, now stop the simulation
        sc_stop();
    }
};

int sc_main(int argc, char *argv[])
{
    try
    {
        // Get the tracefile argument and create Tracefile object
        // This function sets tracefile_ptr and num_cpus
        init_tracefile(&argc, &argv);

        // Initialize statistics counters
        stats_init();

        // Instantiate Modules
        Memory mem("main_memory");
        CPU cpu("cpu");
        // Cache cache("cache");

        // Signals
        sc_buffer<Memory::Function> sigMemFunc;
        sc_buffer<Memory::RetCode> sigMemDone;

        // // extra sig for cache
        // sc_buffer<Cache::Function> sigCacheFunc;
        // sc_buffer<Cache::RetCode> sigCacheDone;
        // // sigs between cache and mem
        sc_signal<uint64_t> sigMemAddr;
        //  represents a 64-bit vector. The _rv suffix indicates that it is a resolved vector signal, which can be used to represent a 64-bit data bus or a vector of bits related to memory data.
        sc_signal_rv<64> sigMemData;

        // The clock that will drive the CPU and Memory
        sc_clock clk;

        // Connecting module ports with signals
        mem.Port_Func(sigMemFunc);
        mem.Port_Addr(sigMemAddr);
        mem.Port_Data(sigMemData);
        mem.Port_Done(sigMemDone);

        // cache.Port_Func(sigMemFunc);
        // cache.Port_Addr(sigMemAddr);
        // cache.Port_Data(sigMemData);
        // cache.Port_Done(sigMemDone);

        cpu.Port_MemFunc(sigMemFunc);
        cpu.Port_MemAddr(sigMemAddr);
        cpu.Port_MemData(sigMemData);
        cpu.Port_MemDone(sigMemDone);

        mem.Port_CLK(clk);
        // cache.Port_CLK(clk);
        cpu.Port_CLK(clk);

        cout << "Running (press CTRL+C to interrupt)... " << endl;
        // mem.dump(); // Uncomment to dump memory to stdout.

        // Start Simulation
        sc_start();
        // TODO: Cache actions and cache state transitions should be printed to the console.

        // Print statistics after simulation finished
        stats_print();
        mem.dump_cache(); // Uncomment to dump memory to stdout.
    }

    catch (exception &e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}
