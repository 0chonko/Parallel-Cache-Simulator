#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <systemc.h>

#include "Memory.h"
#include "cpu_cache_if.h"
#include "bus_master_if.h"

#include "helpers.h"
#include "types.h"

// Class definition without the SC_ macro because we implement the
// cpu_cache_if interface.
class Cache : public cpu_cache_if, public bus_master_if, sc_module {
    // struct CacheLine
    // {
    //     uint64_t tag;
    //     uint32_t data[8]; // 32 byte line
    //     bool state;
    //     bool dirty;
    // };

    // struct CacheSet
    // {
    //     CacheLine lines[8];   // 8-way
    //     uint8_t agingBits[8]; // 3-bit aging bits for each line, stored as 8-bit integers
    // };

    //     // Define the number of bits for the set index and block offset
    // static const int SET_INDEX_BITS = 7;
    // static const int BLOCK_OFFSET_BITS = 5;
    // // static const int TAG_BITS = 64 - SET_INDEX_BITS - BLOCK_OFFSET_BITS;

    // // Calculate the masks for the set index and block offset
    // static const uint64_t setIndexMask = (1 << SET_INDEX_BITS) - 1;
    // static const uint64_t blockOffsetMask = (1 << BLOCK_OFFSET_BITS) - 1;
    // // static const uint64_t setTagMask = (1 << TAG_BITS) - 1;
    // // static const uint64_t tagMask = ~(setIndexMask | blockOffsetMask);

    // CacheSet *cache;



    public:
    // sc_port<bus_slave_if> memory; //cache to bus
        sc_port<bus_master_if> bus; // use this to communicate to BUS
        sc_in_clk clock; // Input for clock signal

        int cpu_read(uint64_t addr);
        int cpu_write(uint64_t addr);
        int read_to_bus(uint64_t addr);
        int write_to_bus(uint64_t addr);
        void update_aging_bits(uint64_t setIndex, uint64_t blockOffset);
        bool containsZero(uint8_t *agingBits);
        int find_oldest(uint64_t setIndex);
        // void handle_snoop();
        // void handle_clock();

    // sc_in<snoop_message> snooping_signal; // Input for snooping/broadcast messages 
    // TODO: make sure the issuing cache doesnt invalidate itself

    // SC_CTOR(Cache, sc_module_name name_, int id_) : sc_module(name_), id(id_) {
    //     SC_METHOD(handle_snoop);
    //     sensitive << snooping_signal.value_changed_event();

    //     SC_METHOD(handle_clock);
    //     sensitive << clock.pos();

    //     dont_initialize();
    // }

    Cache(sc_module_name name_, int id_) : sc_module(name_), id(id_) {
        // Passive for now, just handle cpu requests.
    }

    ~Cache() {
    }

    private:
    int id;
};

#endif
