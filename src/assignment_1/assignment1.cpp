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

SC_MODULE(Memory) {
public:
    enum Function { FUNC_READ, FUNC_WRITE };
    enum RetCode { RET_READ_DONE, RET_WRITE_DONE };

    struct CacheLine {
        uint64_t tag;
        uint32_t data[8];
        bool state, dirty;
    };

    struct CacheSet {
        CacheLine lines[8];
        uint8_t agingBits[8];
    };

    sc_in<bool> Port_CLK;
    sc_in<Function> Port_Func;
    sc_in<uint64_t> Port_Addr;
    sc_out<RetCode> Port_Done;
    sc_inout_rv<64> Port_Data;

    SC_CTOR(Memory) : m_data(new uint64_t[CACHE_SIZE]), cache(new CacheSet[SET_COUNT]) {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
    }

    ~Memory() { delete[] m_data; }

    void dump() {
        for (size_t i = 0; i < MEM_SIZE; i++) {
            cout << setw(5) << i << ": " << setw(5) << m_data[i];
            if (i % 8 == 7) cout << endl;
        }
    }

    void dump_cache() {
        for (size_t i = 0; i < SET_COUNT; i++) {
            for (size_t j = 0; j < 8; j++) {
                for (size_t k = 0; k < 8; k++) {
                    size_t index = (i * 8 * 8) + (j * 8) + k;
                    if (index < CACHE_SIZE) {
                        cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].data[k];
                    }
                    if (index % 8 == 7) cout << endl;
                }
            }
        }
    }

private:
    uint64_t *m_data;
    CacheSet *cache;

    const int SET_INDEX_BITS = 7;
    const int BLOCK_OFFSET_BITS = 5;
    const int TAG_BITS = 64 - SET_INDEX_BITS - BLOCK_OFFSET_BITS;

    uint64_t setIndexMask = (1ULL << SET_INDEX_BITS) - 1;
    uint64_t blockOffsetMask = (1ULL << BLOCK_OFFSET_BITS) - 1;
    uint64_t setTagMask = ((1ULL << SET_INDEX_BITS) - 1) << BLOCK_OFFSET_BITS;

    void update_aging_bits(uint64_t setIndex, uint64_t blockOffset) {
        for (int i = 0; i < 8; i++) {
            if (i != blockOffset && cache[setIndex].agingBits[i] > 1) cache[setIndex].agingBits[i] -= 1;
            else cache[setIndex].agingBits[i] = 8;
        }
    }

    // bool containsZero(uint8_t * agingBits) { return std::any_of(agingBits, agingBits + 8,  { return i == 0; }); }

    int find_oldest(uint64_t setIndex) { return std::distance(cache[setIndex].agingBits, std::min_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8)); }

    void execute() {
        while (true) {
            wait(Port_Func.value_changed_event());
            Function f = Port_Func.read();
            uint64_t addr = Port_Addr.read();
            uint64_t data = 0;
            uint32_t ret_data = 0;

            uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
            uint64_t blockOffset = addr & blockOffsetMask;
            uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);

            if (f == FUNC_WRITE) {  
                bool tagMatch = false;
                data = Port_Data.read().to_uint64();
                for (int i = 0; i < 8; i++) {
                    if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
                        stats_writehit(0);
                        tagMatch = true;
                        cache[setIndex].lines[i].data[blockOffset] = data;
                        cache[setIndex].lines[i].tag = tag;
                        update_aging_bits(setIndex, i);
                        break;
                    }
                }
                if (!tagMatch) {
                    stats_writemiss(0);
                    int oldest = find_oldest(setIndex);
                    if (cache[setIndex].lines[oldest].dirty) cache[setIndex].lines[oldest].dirty = 0;
                    wait(100);
                    if (oldest == 0) oldest = 1;
                    cache[setIndex].lines[oldest].state = 1;
                    cache[setIndex].lines[oldest].tag = tag;
                    cache[setIndex].lines[oldest].data[blockOffset] = data;
                    update_aging_bits(setIndex, oldest);
                }
                wait(1);
            } else {
                bool tagMatch = false;
                for (int i = 0; i < 8; i++) {
                    if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
                        stats_readhit(0);
                        tagMatch = true;
                        ret_data = cache[setIndex].lines[i].data[blockOffset];
                        cache[setIndex].lines[i].tag = tag;
                        update_aging_bits(setIndex, i);
                        break;
                    }
                }
                if (!tagMatch) {
                    stats_readmiss(0);
                    int oldest = find_oldest(setIndex);
                    if (cache[setIndex].lines[oldest].dirty) cache[setIndex].lines[oldest].dirty = 0;
                    wait(100);
                    if (oldest == 0) oldest = 1;
                    cache[setIndex].lines[oldest].state = 1;
                    cache[setIndex].lines[oldest].tag = tag;
                    ret_data = cache[setIndex].lines[oldest].data[blockOffset];
                    update_aging_bits(setIndex, oldest);
                }
                wait(1);
            }

            if (f == FUNC_READ) {
                Port_Data.write((addr < CACHE_SIZE) ? ret_data : 0);
                Port_Done.write(RET_READ_DONE);
                wait();
                Port_Data.write(float_64_bit_wire);
            } else {
                Port_Done.write(RET_WRITE_DONE);
            }
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
                break;

            case TraceFile::ENTRY_TYPE_WRITE:
                f = Memory::FUNC_WRITE;
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
        // mem.dump_cache(); // Uncomment to dump memory to stdout.
    }

    catch (exception &e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}
