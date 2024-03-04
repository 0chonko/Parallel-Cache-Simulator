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
#include "global_vars.h"


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
                cout << "reached 1" << endl;

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
            cpus[i]->clock(clk);     // connect cpus to clock
            caches[i]->clock(clk);   // connect caches to clock
            bus->clock(clk);         // connect bus to clock
        }

        // Connect caches to the bus
        for (int i = 0; i < NUM_CACHES; i++) {
            cpus[i]->cache(*caches[i]); // connect cpus to caches
            // Connect Cache to Bus
            caches[i]->bus(bus->cache_ports[i]);         
            // caches[i]->bus_channel(bus->broadcast_channel); // connect caches to bus
        }
        // Connect bus to memory
        bus->memory(*memory);

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
