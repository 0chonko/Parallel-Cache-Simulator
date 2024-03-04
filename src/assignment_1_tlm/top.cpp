/*
// File: top.cpp
//
*/

#include <iostream>
#include <systemc.h>

#include "CPU.h"
#include "Cache.h"
#include "Memory.h"
#include "Bus.h"
#include "psa.h"

using namespace std;

int sc_main(int argc, char *argv[]) {
    try {
        // Get the tracefile argument and create Tracefile object
        // This function sets tracefile_ptr and num_cpus
        init_tracefile(&argc, &argv);

        // init_tracefile changed argc and argv so we cannot use
        // getopt anymore.
        // The "-q" flag must be specified _after_ the tracefile.
        if (argc == 2 && !strcmp(argv[0], "-q")) {
            sc_report_handler::set_verbosity_level(SC_LOW);
        }

        sc_set_time_resolution(1, SC_PS);

        // Initialize statistics counters
        stats_init();

        // Automatically determine the number of CPUs and caches from the tracefiles
        // TDOD: get the number of CPUs from the tracefile
        const int NUM_CPUS = 2;
        const int NUM_CACHES = NUM_CPUS;

        CPU *cpus[NUM_CPUS];
        Cache *caches[NUM_CACHES];

        for (int i = 0; i < NUM_CPUS; i++) {
            cpus[i] = new CPU(sc_gen_unique_name("cpu"), i);
        }

        for (int i = 0; i < NUM_CACHES; i++) {
            caches[i] = new Cache(sc_gen_unique_name("cache"), i);
        }
        Memory *memory = new Memory("memory");
        Bus *bus = new Bus("bus");


        // The clock that will drive the CPU
        sc_clock clk;

        // Connect instances
        for (int i = 0; i < NUM_CPUS; i++) {
            cpus[i]->cache(*caches[i]);
            cpus[i]->clock(clk);
        }

        // Connect bus to memory
        bus->memory(*memory);

        // Create signals for communication between cache and bus
        sc_signal<bool> cache_request[NUM_CACHES];
        sc_signal<bool> cache_response[NUM_CACHES];
        sc_signal<sc_uint<ADDRESS_WIDTH>> address[NUM_CACHES]; // Assuming ADDRESS_WIDTH is defined.
        sc_signal<data_t> data_to_cache[NUM_CACHES]; // Assuming data_t is a data type for cache lines.
        sc_signal<data_t> data_from_cache[NUM_CACHES];

        // Connect caches to the bus
        for (int i = 0; i < NUM_CACHES; i++) {
            // Connect Cache to Bus
            caches[i]->bus(*bus);
            // Bind cache signals to bus signals
            bus->cache_request[i](cache_request[i]);
            bus->cache_response[i](cache_response[i]);
            bus->address[i](address[i]);
            bus->data_to_cache[i](data_to_cache[i]);
            bus->data_from_cache[i](data_from_cache[i]);
        }

        // Start Simulation
        sc_start();

        // Print statistics after simulation finished
        stats_print();
        cout << sc_time_stamp() << endl;

        // Cleanup components
        for (int i = 0; i < NUM_CPUS; i++) {
            delete cpus[i];
            delete caches[i];
        }
        delete memory;
        delete bus;

    } catch (exception &e) {
        cerr << e.what() << endl;
    }

    return 0;
}
