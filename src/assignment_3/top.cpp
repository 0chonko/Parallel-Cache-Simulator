/*
// File: top.cpp
//
*/

#include <iostream>
#include <systemc.h>

#include "CPU.h"
#include "Cache.h"
#include "psa.h"
#include "Memory.h"
#include "Bus.h"

using namespace std;
using namespace sc_core;    

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
        const int NUM_CPUS = num_cpus;
        const int NUM_CACHES = NUM_CPUS;

        CPU *cpus[NUM_CPUS];
        std::vector<Cache*> caches(NUM_CACHES);


        for (int i = 0; i < NUM_CPUS; i++) {
            cpus[i] = new CPU(sc_gen_unique_name("cpu"), i);
        }

        for (int i = 0; i < NUM_CACHES; i++) {
            caches[i] = new Cache(sc_gen_unique_name("cache"), i);
        }
        
        Memory *memory  = new Memory("memory");
        Bus *bus        = new Bus("bus");


        // The clock that will drive the CPU
        sc_clock clk;
        for (int i = 0; i < NUM_CPUS; i++) {
            cpus[i]->clock(clk);
            caches[i]->clock(clk);
            // Connect instances
            cpus[i]->cache(*caches[i]);
            caches[i]->bus(*bus);
            // caches[i]->in(bus->out);
        }
        bus->memory(*memory);
        memory->bus(*bus);
        bus->clock(clk);

        // for (int i = 0; i < NUM_CACHES; i++) {
        bus->caches = caches;


        cout << "Starting simulation" << endl;

        // Start Simulation
        sc_start();

        // Print statistics after simulation finished
        stats_print();
        cout << sc_time_stamp() << endl;

        //dump caches
        for (int i = 0; i < NUM_CACHES; i++) {
            caches[i]->dump_cache();
        }

        // Cleanup components
        for (int i = 0; i < NUM_CPUS; i++) {
            delete cpus[i];
        }
        for (int i = 0; i < NUM_CACHES; i++) {
            delete caches[i];
        }
        delete memory;
        delete bus;
    } catch (exception &e) {
        cerr << e.what() << endl;
    }

    return 0;
}
