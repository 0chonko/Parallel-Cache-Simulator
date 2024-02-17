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

#include "psa.h"

using namespace std;
using namespace sc_core; // This pollutes namespace, better: only import what you need.

static const size_t MEM_SIZE = 2500;
static const size_t CACHE_SIZE = 32000;
static const size_t LINE_SIZE = 4; //* sizeof(uint64_t);
static const size_t ADDR_SIZE = 32;
// static const size_t LINE_NUM = 1000;

// TODO: 8-way associative - 8 cachelines per operation - 32000 / 32 = 1000 cache line => 1000 / 8 = 125 sets 
// TODO: least-recently-used write-back replacement strategy together with an allocate-on-write policy.
// whole cache line of 32 bytes can be transferred with a single read or write

 
SC_MODULE(Cache) {
    public:

    enum Function { FUNC_READ, FUNC_WRITE };
    enum RetCode { RET_READ_DONE, RET_WRITE_DONE };

    // signal in
    sc_in<bool> Port_CLK;
    sc_in<Function> Port_Func;
    sc_in<uint64_t> Port_Addr;

    // signal out
    sc_out<RetCode> Port_Done;

    // signal inout
    sc_inout_rv<64> Port_Data;

    SC_CTOR(Cache) {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();

        m_data = new uint64_t[MEM_SIZE];
    }

    ~Cache() {
        delete[] m_data;
    }

    void dump() {
        for (size_t i = 0; i < MEM_SIZE; i++) {
            cout << setw(5) << i << ": " << setw(5) << m_data[i];
            if (i % 8 == 7) {
                cout << endl;
            }
        }
    }

    private:
    uint64_t *m_data;
    // convert incoming address to set
    size_t addressModuloOperation(uint64_t address) {
        return Port_Addr % ((CACHE_SIZE / (sizeof(uint64_t) * 4))  / 8);
    }

    void execute() {
        while (true) {
            wait(Port_Func.value_changed_event());

            Function f = Port_Func.read();
            uint64_t addr = Port_Addr.read();
            uint64_t data = 0;
            if (f == FUNC_WRITE) {
                cout << sc_time_stamp() << ": CACHE received write" << endl;
                data = Port_Data.read().to_uint64();
            } else {
                cout << sc_time_stamp() << ": CACHE received read" << endl;
            }
            cout << sc_time_stamp() << ": CACHE address " << addr << endl;

            // This simulates cache read/write delay of the cache
            wait(1);

            if (f == FUNC_READ) {
                Port_Data.write((addr < MEM_SIZE) ? m_data[addr] : 0);
                Port_Done.write(RET_READ_DONE);
                wait();
                Port_Data.write(float_64_bit_wire); // string with 64 "Z"'s
            } else {
                if (addr < MEM_SIZE) {
                    m_data[addr] = data;
                }
                Port_Done.write(RET_WRITE_DONE);
            }
        }
    }
};

SC_MODULE(Memory) {
    public:

    enum Function { FUNC_READ, FUNC_WRITE };
    enum RetCode { RET_READ_DONE, RET_WRITE_DONE };

    // signal in
    sc_in<bool> Port_CLK;
    sc_in<Function> Port_Func;
    sc_in<uint64_t> Port_Addr;

    // signal out
    sc_out<RetCode> Port_Done;

    // signal inout
    sc_inout_rv<64> Port_Data;

    SC_CTOR(Memory) {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();

        m_data = new uint64_t[MEM_SIZE];
    }

    ~Memory() {
        delete[] m_data;
    }

    void dump() {
        for (size_t i = 0; i < MEM_SIZE; i++) {
            cout << setw(5) << i << ": " << setw(5) << m_data[i];
            if (i % 8 == 7) {
                cout << endl;
            }
        }
    }

    private:
    uint64_t *m_data;

    void execute() {
        while (true) {
            wait(Port_Func.value_changed_event());

            Function f = Port_Func.read();
            uint64_t addr = Port_Addr.read();
            uint64_t data = 0;
            if (f == FUNC_WRITE) {
                cout << sc_time_stamp() << ": MEM received write" << endl;
                data = Port_Data.read().to_uint64();
            } else {
                cout << sc_time_stamp() << ": MEM received read" << endl;
            }
            cout << sc_time_stamp() << ": MEM address " << addr << endl;

            // This simulates memory read/write delay
            wait(100);

            if (f == FUNC_READ) {
                Port_Data.write((addr < MEM_SIZE) ? m_data[addr] : 0);
                Port_Done.write(RET_READ_DONE);
                wait();
                Port_Data.write(float_64_bit_wire); // string with 64 "Z"'s
            } else {
                if (addr < MEM_SIZE) {
                    m_data[addr] = data;
                }
                Port_Done.write(RET_WRITE_DONE);
            }
        }
    }
};

SC_MODULE(CPU) {
    public:
    sc_in<bool> Port_CLK;
    sc_in<Memory::RetCode> Port_MemDone;
    sc_out<Memory::Function> Port_MemFunc;
    sc_out<uint64_t> Port_MemAddr;
    sc_inout_rv<64> Port_MemData;

    SC_CTOR(CPU) {
        SC_THREAD(execute);
        sensitive << Port_CLK.pos();
        dont_initialize();
    }

    private:
    void execute() {
        TraceFile::Entry tr_data;
        Memory::Function f;

        // Loop until end of tracefile
        while (!tracefile_ptr->eof()) {
            // Get the next action for the processor in the trace
            if (!tracefile_ptr->next(0, tr_data)) {
                cerr << "Error reading trace for CPU" << endl;
                break;
            }

            // To demonstrate the statistic functions, we generate a 50%
            // probability of a 'hit' or 'miss', and call the statistic
            // functions below
            int j = rand() % 2;

            switch (tr_data.type) {
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

            case TraceFile::ENTRY_TYPE_NOP: break;

            default:
                cerr << "Error, got invalid data from Trace" << endl;
                exit(0);
            }

            if (tr_data.type != TraceFile::ENTRY_TYPE_NOP) {
                Port_MemAddr.write(tr_data.addr);
                Port_MemFunc.write(f);

                if (f == Memory::FUNC_WRITE) {
                    cout << sc_time_stamp() << ": CPU sends write" << endl;

                    // We write the address as the data value.
                    Port_MemData.write(tr_data.addr);
                    wait();
                    Port_MemData.write(float_64_bit_wire); // 64 "Z"'s

                } else {
                    cout << sc_time_stamp() << ": CPU sends read" << endl;
                }

                wait(Port_MemDone.value_changed_event());

                if (f == Memory::FUNC_READ) {
                    cout << sc_time_stamp()
                         << ": CPU reads: " << Port_MemData.read() << endl;
                }
            } else {
                cout << sc_time_stamp() << ": CPU executes NOP" << endl;
            }
            // Advance one cycle in simulated time
            wait();
        }

        // Finished the Tracefile, now stop the simulation
        sc_stop();
    }
};


int sc_main(int argc, char *argv[]) {
    try {
        // Get the tracefile argument and create Tracefile object
        // This function sets tracefile_ptr and num_cpus
        init_tracefile(&argc, &argv);

        // Initialize statistics counters
        stats_init();

        // Instantiate Modules
        Memory mem("main_memory");
        CPU cpu("cpu");
        Cache cache("cache");

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

        mem.Port_Func(sigMemFunc);
        mem.Port_Addr(sigMemAddr);
        mem.Port_Data(sigMemData);
        mem.Port_Done(sigMemDone);

        cpu.Port_MemFunc(sigMemFunc);
        cpu.Port_MemAddr(sigMemAddr);
        cpu.Port_MemData(sigMemData);
        cpu.Port_MemDone(sigMemDone);

        
        mem.Port_CLK(clk);
        cpu.Port_CLK(clk);

        cout << "Running (press CTRL+C to interrupt)... " << endl;


        // Start Simulation
        sc_start();
        // TODO: Cache actions and cache state transitions should be printed to the console.

        // Print statistics after simulation finished
        stats_print();
        // mem.dump(); // Uncomment to dump memory to stdout.
    }

    catch (exception &e) {
        cerr << e.what() << endl;
    }

    return 0;
}
