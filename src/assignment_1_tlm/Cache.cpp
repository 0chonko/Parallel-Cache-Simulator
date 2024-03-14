#include <assert.h>
#include <systemc.h>
#include <iostream> // Required for cout
#include <iomanip>  // Required for setw
#include <algorithm> // Required for any_of and min_element
#include <cstdint>   // Required for fixed width integer types

#include "Cache.h"
#include "bus_master_if.h"
#include "psa.h"
// #include "types.h"

// sc_port<bus_slave_if> cache;

// sc_channel<snoop_message> *bus_channel;


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
    VALID,
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
CacheSet* cache = new CacheSet[SET_COUNT];
bool debug = true;



// dumps the content of the cache
void dump_cache() {
    using std::cout;
    using std::endl;
    using std::setw;

    for (size_t i = 0; i < SET_COUNT; i++) {
        // print every line in every set in the cache
        for (size_t j = 0; j < 8; j++) {
            for (size_t k = 0; k < 8; k++) {
                size_t index = (i * 8 * 8) + (j * 8) + k;
                if (index < CACHE_SIZE) {
                    cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].data[k];
                    if (index % 8 == 7) {
                        cout << endl;
                    }
                } else {
                    break;
                }
            }
        }
    }
}

// Define the number of bits for the set index and block offset
const int SET_INDEX_BITS = 7;
const int BLOCK_OFFSET_BITS = 5;
// const int TAG_BITS = 64 - SET_INDEX_BITS - BLOCK_OFFSET_BITS;

// Calculate the masks for the set index and block offset
uint64_t setIndexMask = (1ULL << SET_INDEX_BITS) - 1;
uint64_t blockOffsetMask = (1ULL << BLOCK_OFFSET_BITS) - 1;
// uint64_t tagMask = (1ULL << TAG_BITS) - 1;

void Cache::update_aging_bits(uint64_t setIndex, uint64_t lineIndex) {
    for (uint64_t i = 0; i < 8; i++) {
        if (i != lineIndex) {
            if (cache[setIndex].agingBits[i] < 7 && cache[setIndex].lines[i].state != INVALID) {
                cache[setIndex].agingBits[i] += 1;
            }
        } else {
            cache[setIndex].agingBits[i] = 0; // set as freshest entry
        }
    }
}

bool Cache::containsZero(uint8_t* agingBits) {
    return std::any_of(agingBits, agingBits + 8, [](int i) { return i == 0; });
}

int Cache::find_oldest(uint64_t setIndex) {
    return std::distance(cache[setIndex].agingBits, std::max_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8));
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_read(uint64_t addr) {
    wait();

    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t blockOffset = addr & blockOffsetMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);


    bool tagMatch = false;

    // READ HIT
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            tagMatch = true;
            log(name(), "read hit because incoming tag is ", tag, " and cache tag is ", cache[setIndex].lines[i].tag, " and state is ", cache[setIndex].lines[i].state);
            handle_read_hit(addr, setIndex, tag, i);
            break;
        } 
    }

    // READ MISS
    if (!tagMatch) { //TODO: or if state was invalid?
        handle_read_miss(addr, setIndex, tag);
    }
    wait(1);
    RET_RESPONSE = false;
    return 0; // Done, return value 0 signals succes.
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_write(uint64_t addr) {
    wait();

    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);

    bool tagMatch = false;

    // WRITE HIT
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            tagMatch = true;
            cache[setIndex].lines[i].tag = tag;
            handle_write_hit(addr, setIndex, tag, i);
            break;
        } 
    }

    // WRITE MISS
    if (!tagMatch) { // TODO: or if state was invalid
        // log(name(), "write miss on address", addr);
        handle_write_miss(addr, setIndex, tag); // TODO: test write miss  
    }
    wait(1);
    RET_RESPONSE = false;

    return 0; // indicates succes.
}

int Cache::read_snoop(uint64_t addr, bool isWrite) {
    // log(name(), "snoop request on address", addr);
        int invalidation_line = has_cacheline(addr);

        if (invalidation_line != -1) {
            uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
            uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
            log(name(), "invalidated address", addr);
            // cache[setIndex].lines[i].state = INVALID; // THIS IS UPDATED
            if (isWrite) {
            handle_probe_write_hit(addr, setIndex, tag, invalidation_line); // TODO: UPDATES STATE ACCORDINGLY IF THERE'S SOMETHING TO UDPATE
            } else {
                handle_probe_read_hit(addr, 0, 0);
            }
    }
    wait(1);
    return 0;
}


bool Cache::wait_for_response(uint64_t addr, int id) {
    // log(name(), "waiting for response from memory", addr);

    wait(memory_write_event | status_update_event);

    if (memory_write_event.triggered()) {
        cout << "memory_write_event has triggered" << endl;
        return true;
    }
    if (status_update_event.triggered()) {
        cout << "status_update_event has triggered" << endl;
        return false;
    }
    // log(name(), "received response from memory", addr);
}

void Cache::response_received(uint64_t addr, int id) {
    RET_RESPONSE = true;
    // wait(1);
}

int Cache::has_cacheline(uint64_t addr) { //TODO should return the line index which is later used to get the state
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) { //TODO: this might have broken
            return i;
        }
    }
    return -1;
}

void Cache::handle_write_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex) { //TODO: wrong memory access count
    // bus ->snoop(addr, id, 1);
    cout << "############## ENTER WRITE HIT WITH STATE" << cache[setIndex].lines[matchedLineIndex].state << endl;
    bus->request(addr, true, id, true); // write-through
    wait_for_response(addr, id); 
    wait();
    log(name(), "write hit");


    if (cache[setIndex].lines[matchedLineIndex].state == EXCLUSIVE) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from EXCLUSIVE to MODIFIED", addr);
    } else if (cache[setIndex].lines[matchedLineIndex].state == MODIFIED) { // done
        log(name(), "write hit, MODIFIED stays", addr);
    } else if (cache[setIndex].lines[matchedLineIndex].state == SHARED) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from SHARED to MODIFIED", addr);
    } else if (cache[setIndex].lines[matchedLineIndex].state == OWNED) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from OWNED to MODIFIED", addr);
    }
    
    if (cache[setIndex].lines[matchedLineIndex].state != INVALID) {
        stats_writehit(id);
        update_aging_bits(setIndex, matchedLineIndex);

     } else {
        handle_write_miss(addr, setIndex, tag);
     }
    

}

// it should miss if some other processor invalidates the entry while the bus request is in flight
// before sending snoop
void Cache::handle_write_miss(uint64_t addr, int setIndex, int tag) {
    log(name(), "write miss on address", addr);
    if (bus->busy()) {
        cout << "BUSY" << endl;
    }
    bus->request(addr, false, id, false); // pull data
    wait_for_response(addr, id); 
    int oldest = find_oldest(setIndex);
    wait();

    // cache[setIndex].lines[oldest].state = VALID;
    // cache[setIndex].lines[oldest].tag = tag;

    if (cache[setIndex].lines[oldest].state == INVALID) {
        cache[setIndex].lines[oldest].state = MODIFIED;
        log(name(), "write miss, from INVALID to MODIFIED", addr);
    } else if (cache[setIndex].lines[oldest].state == EXCLUSIVE) {
        cache[setIndex].lines[oldest].state = MODIFIED;
        log(name(), "write miss, from EXCLUSIVE to MODIFIED", addr);
    } else if (cache[setIndex].lines[oldest].state == SHARED) {
        cache[setIndex].lines[oldest].state = MODIFIED;
        log(name(), "write miss, from SHARED to MODIFIED", addr);
    } else if (cache[setIndex].lines[oldest].state == OWNED) {
        cache[setIndex].lines[oldest].state = MODIFIED;
        log(name(), "write miss, from OWNED to MODIFIED", addr);
    }

    bus->request(addr, true, id, false); //write-through
    wait_for_response(addr, id);
    wait();

    update_aging_bits(setIndex, oldest);


    stats_writemiss(id);

}

void Cache::handle_read_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex) { // Nothing to do
    // uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    log(name(), "read hit on address ", addr, " SET ", setIndex);
    bus->request(addr, false, id, true);
    wait_for_response(addr, id); 
    stats_readhit(id);
    wait();

}

void Cache::handle_read_miss(uint64_t addr, int setIndex, int tag) {
    // uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    log(name(), "read miss on address ", addr, " SET ", setIndex);
    int oldest = find_oldest(setIndex);


    bus->request(addr, false, id, false);
    wait_for_response(addr, id); 

    // Print or process the message
    wait();
    
    // cache[setIndex].lines[oldest].state = VALID; // TODO: is it tho?
    // // log(name(), "cacheline valid again: ", addr);
    // cache[setIndex].lines[oldest].tag = tag;
    // update_aging_bits(setIndex, oldest);

    //     read miss 
    //         -> if available in another cache get it and set to SHARED 
    //         -> otherwise get from memory and set EXCLUSIVE
    //         -> INVALID to EXCLUSIVE or to SHARED (if other present)

    if (cache[setIndex].lines[oldest].state == INVALID) {
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        log(name(), "read miss, from INVALID to EXCLUSIVE", addr);
    } else if (cache[setIndex].lines[oldest].state == EXCLUSIVE) {
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        log(name(), "read miss, stay EXCLUSIVE", addr);
    } 

    update_aging_bits(setIndex, oldest);

    stats_readmiss(id);

}

void Cache::handle_probe_write_hit(uint64_t addr, int setIndex, int tag, int i) {
    if (cache[setIndex].lines[i].state == EXCLUSIVE) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe write hit, from EXCLUSIVE to SHARED", addr);
    } else if (cache[setIndex].lines[i].state == MODIFIED) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe write hit, from MODIFIED to SHARED", addr);
    } else if (cache[setIndex].lines[i].state == OWNED) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe write hit, from OWNED to SHARED", addr);
    }
}

void Cache::handle_probe_read_hit(uint64_t addr, int setIndex, int tag) { //TODO add line index
    if (cache[setIndex].lines[0].state == EXCLUSIVE) {
        cache[setIndex].lines[0].state = SHARED;
        log(name(), "probe read hit, from EXCLUSIVE to SHARED", addr);
    } else if (cache[setIndex].lines[0].state == MODIFIED) {
        cache[setIndex].lines[0].state = SHARED;
        log(name(), "probe read hit, from MODIFIED to SHARED", addr);
    } else if (cache[setIndex].lines[0].state == OWNED) {
        cache[setIndex].lines[0].state = SHARED;
        log(name(), "probe read hit, from OWNED to SHARED", addr);
    }
}

void Cache::handle_eviction(uint64_t addr, int setIndex, int tag, int evictionLineIndex) {
    // log(name(), "evicting line", addr);
    // cache[setIndex].lines[evictionLineIndex].state = INVALID;
    // log(name(), "evicted line", addr);
}

// wipe the data when the class instance is destroyed as a member function



// This cache hits or misses:
//     read/write miss 
//         -> pull memory or another cache_request
//     read miss 
//         -> if available in another cache get it and set to SHARED 
//         -> otherwise get from memory and set EXCLUSIVE
//         -> INVALID to EXCLUSIVE or to SHARED (if other present)

//     write miss 
//         -> get from other cache or memory and set to MODIFIED
//         -> if INVALID write back and to MODIFIED

//     read hit
//         -> stay same State

//     write hit 
//         -> from EXCLUSIVE set to MODIFIED and invalidate other copies
//         -> MODIFIED stays
//         -> from SHARED to MODIFIED
//         -> from OWNED to MODIFIED

// IN (incoming snoop and has_cacheLine is true):
//     read hit 
//         -> from MODIFIED to OWNED
//         -> from SHARED stay SHARED
//         -> from EXCLUSIVE to SHARED
//         -> stay in OWNED
//     write hit
//         -> from EXCLUSIVE to INVALID
//         -> from MODIFIED to INVALID
//         -> from SHARED to INVALID
//         -> from OWNED to INVALID