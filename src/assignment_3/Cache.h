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
    bool containsZero(uint8_t* agingBits);
    int find_oldest(uint64_t setIndex);
    int read_snoop(uint64_t addr, bool isWrite);
    int get_max_oldest(uint64_t setIndex);
    int has_cacheline(uint64_t addr, bool isWrite);
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
        cache = new CacheSet[SET_COUNT];
    }

    SC_HAS_PROCESS(Cache);


    ~Cache() {
    }

private:
    int id;
    static const size_t MEM_SIZE = 2500;
    // static const size_t CACHE_SIZE = 16384; // 4kb   
    static const size_t CACHE_SIZE = 32768;
    // static const size_t CACHE_SIZE = 65536; // 64kb
    // static const size_t CACHE_SIZE = 131072; // 128kb 
    // static const size_t CACHE_SIZE = 524288; // 512kb
    static const size_t LINES_COUNT = CACHE_SIZE / 32;
    static const size_t LINE_SIZE = 32; //* sizeof(uint64_t);
    static const size_t SET_SIZE = 8;
    static const size_t SET_COUNT = LINES_COUNT / SET_SIZE;
    bool RET_RESPONSE = false;

    enum CacheState {
        INVALID,
        EXCLUSIVE, // only one cache can have this line
        SHARED, // multiple caches can have this line
        MODIFIED, // only one cache can have this line, and it's different from the memory
        OWNED // multiple caches can have this line, and it's different from the memory
    };

    struct CacheLine {
        uint64_t tag;
        uint32_t data[8]; // 32 byte line
        CacheState state = INVALID; // initialize state to INVALID
        // bool dirty;
    };

    struct CacheSet {
        CacheLine lines[8]; // 8-way
        uint8_t agingBits[8]; // 3-bit aging bits for each line, stored as 8-bit integers
    };

    // Initialization should use 'new[]' for arrays
    CacheSet* cache;
};

#endif