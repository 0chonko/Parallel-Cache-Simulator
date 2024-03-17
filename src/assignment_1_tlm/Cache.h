#ifndef CACHE_H
#define CACHE_H
#include <iostream>
#include <systemc.h>
#include "Memory.h"
#include "cpu_cache_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "psa.h"

// Class definition without the SC_ macro because we implement the
// cpu_cache_if interface.
class Cache : public cpu_cache_if, sc_module {
    public:
        sc_port<bus_master_if> bus; // use this to communicate to BUS
        sc_in_clk clock;
        sc_event memory_write_event;
        sc_event status_update_event;


        int cpu_read(uint64_t addr);
        int cpu_write(uint64_t addr);
        void dump_cache();
        void update_aging_bits(uint64_t setIndex, uint64_t blockOffset);
        bool containsZero(uint8_t *agingBits);
        int find_oldest(uint64_t setIndex);
        int read_snoop(uint64_t addr, bool isWrite);
        int has_cacheline(uint64_t addr);
        void response_received(uint64_t addr, int id);
        bool wait_for_response(uint64_t addr, int id);
        void handle_write_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex);
        void handle_write_miss(uint64_t addr, int setIndex, int tag);
        void handle_read_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex);
        void handle_read_miss(uint64_t addr, int setIndex, int tag);
        void handle_probe_write_hit(uint64_t addr, int setIndex, int tag, int i);
        void handle_probe_read_hit(uint64_t addr, int setIndex, int tag, int i);
        void handle_eviction(uint64_t addr, int setIndex, int tag, int evictionLineIndex);


        Cache(sc_module_name name_, int id_) : sc_module(name_), id(id_) {
            
        }

        SC_HAS_PROCESS(Cache);


        ~Cache() {
        }

    private:
    int id;
};

#endif