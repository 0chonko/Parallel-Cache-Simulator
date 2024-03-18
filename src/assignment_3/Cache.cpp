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
void Cache::dump_cache() {
    using std::cout;
    using std::endl;
    using std::setw;

    for (size_t i = 0; i < 8; i++) {
        // print every line in every set in the cache
        for (size_t j = 0; j < 8; j++) {
            size_t index = (i * 8) + j;
            if (index < CACHE_SIZE) {
                cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].state;
                if (index % 8 == 7) {
                    cout << endl;
                }
            }
            else {
                cout << endl;
                cout << endl;
                break;
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
    if (cache[setIndex].agingBits[lineIndex] != 0) {  // if the most recent line is accesses again no updates take place
        for (uint64_t i = 0; i < 8; i++) {
            if (i != lineIndex) {
                if (cache[setIndex].agingBits[i] < 7 && cache[setIndex].lines[i].state != INVALID) {
                    cache[setIndex].agingBits[i] += 1;
                }
            }
            else {
                cache[setIndex].agingBits[i] = 0; // set as freshest entry
            }
        }
    }
}

bool Cache::containsZero(uint8_t* agingBits) {
    return std::any_of(agingBits, agingBits + 8, [](int i) { return i == 0; });
}


int Cache::get_max_oldest(uint64_t setIndex) {
    return *max_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8);
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
        log(name(), " read state", cache[setIndex].lines[i].state);
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            tagMatch = true;
            // log(name(), "read hit because incoming tag is ", tag, " and cache tag is ", cache[setIndex].lines[i].tag, " and state is ", cache[setIndex].lines[i].state);
            handle_read_hit(addr, setIndex, tag, i);
            break;
        }
    }

    // READ MISS
    if (!tagMatch) {
        handle_read_miss(addr, setIndex, tag);
    }
    wait();
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
        log(name(), " write state", cache[setIndex].lines[i].state);
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            tagMatch = true;
            handle_write_hit(addr, setIndex, tag, i);
            break;
        }
    }

    // WRITE MISS
    if (!tagMatch) { //TODO: if state was invalid WRITE BACK
        handle_write_miss(addr, setIndex, tag);
    }
    RET_RESPONSE = false;

    return 0; // indicates succes.
}

int Cache::read_snoop(uint64_t addr, bool isWrite) {
    // // log(name(), "snoop request on address", addr);
    //     int invalidation_line = has_cacheline(addr);

    //     if (invalidation_line != -1) {
    //         uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    //         uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    //         log(name(), "read_snoop found a metching data in another cache", addr);
    //         if (isWrite) {
    //             handle_probe_write_hit(addr, setIndex, tag, invalidation_line); // TODO: UPDATES STATE ACCORDINGLY IF THERE'S SOMETHING TO UDPATE
    //         } else {
    //             handle_probe_read_hit(addr, setIndex, tag, invalidation_line);
    //         }
    // }
    // return 0;
}


bool Cache::wait_for_response(uint64_t addr, int id) { // True if no other caches had the data, false otherwise
    // log(name(), "waiting for response from memory", addr);

    wait(memory_write_event | status_update_event);
    // wait();
    if (memory_write_event.triggered()) {
        log(name(), "memory_write_event has triggered");
        return true;
    }
    if (status_update_event.triggered()) {
        log(name(), "status_update_event has triggered");
        return false;
    }
    // log(name(), "received response from memory", addr);
}

void Cache::response_received(uint64_t addr, int id) {
    RET_RESPONSE = true;
    // wait(1);
}

int Cache::has_cacheline(uint64_t addr, bool isWrite) {
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    int matchedLine = -1;
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            log(name(), "FOUND STATE ", cache[setIndex].lines[i].state);
            matchedLine = i;
        }
    }
    if (matchedLine != -1) {
        if (isWrite) {
            log(name(), "has_cacheline found a metching data in another cache", addr);
            handle_probe_write_hit(addr, setIndex, tag, matchedLine);
        }
        else {
            log(name(), "has_cacheline found a metching data in another cache", addr);
            handle_probe_read_hit(addr, setIndex, tag, matchedLine);
        }
    }
    return matchedLine;
}

void Cache::handle_write_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex) { //TODO: wrong memory access count
    //TODO: needs to invalidate other copies 
    log(name(), "write hit address ", addr);

    if (cache[setIndex].lines[matchedLineIndex].state == EXCLUSIVE) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from EXCLUSIVE to MODIFIED", addr);
        wait();

    }
    else if (cache[setIndex].lines[matchedLineIndex].state == MODIFIED) { // done
        log(name(), "write hit, MODIFIED stays", addr);
        wait();

    }
    else if (cache[setIndex].lines[matchedLineIndex].state == SHARED) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from SHARED to MODIFIED", addr);
        wait();

    }
    else if (cache[setIndex].lines[matchedLineIndex].state == OWNED) { // done
        cache[setIndex].lines[matchedLineIndex].state = MODIFIED;
        log(name(), "write hit, from OWNED to MODIFIED", addr);
        wait();
    }
    else {
        log(name(), "write hit, state is INVALID", addr);
    }

    bus->request(addr, true, id, false); // write-through and send probe
    wait_for_response(addr, id);

    stats_writehit(id);
    update_aging_bits(setIndex, matchedLineIndex);
    wait();

    // if (cache[setIndex].lines[matchedLineIndex].state != INVALID) { // TODO: meant to double check if in the meanwhile stuff has been invalidated
    //     stats_writehit(id);
    //     update_aging_bits(setIndex, matchedLineIndex);

    //  } else {
    //     handle_write_miss(addr, setIndex, tag);
    //  }


}

// it should miss if some other processor invalidates the entry while the bus request is in flight
// TODO: on invalidation write back to memory
void Cache::handle_write_miss(uint64_t addr, int setIndex, int tag) {
    int oldest = find_oldest(setIndex);

    if (get_max_oldest(setIndex) == 7) {
        if (cache[setIndex].lines[oldest].state == MODIFIED || cache[setIndex].lines[oldest].state == OWNED) {
            bus->request(addr, true, id, true); // write back
            wait_for_response(addr, id);
            log(name(), "write miss, line written back because MODIFIED or OWNED", addr);
        }
    }


    bus->request(addr, false, id, false); // pull data
    // TODO: should go to either exclusive or shared before going to modified

    if (wait_for_response(addr, id)) { // if true it went to memory
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        log(name(), "allocate on write pulled data and did not find other copies of requested data. Set to EXCLUSIVE", addr);
        bus->request(addr, true, id, false); // TODO: have to check if it was invalidated in the meanwhile
        wait_for_response(addr, id); // TODO: have to check if it was invalidated in the meanwhile by seeing other copies
        cache[setIndex].lines[oldest].state = MODIFIED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "write miss, from memory to MODIFIED", addr);
    }
    else { // other caches have it
        cache[setIndex].lines[oldest].state = SHARED;
        log(name(), "allocate on write pulled data and found other copies of requested data. Set to SHARED", addr);
        bus->request(addr, true, id, false); // invalidate other copies
        wait_for_response(addr, id);
        cache[setIndex].lines[oldest].state = MODIFIED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "write miss, from other cache to MODIFIED", addr);
    }

    update_aging_bits(setIndex, oldest);
    stats_writemiss(id);


}

void Cache::handle_read_hit(uint64_t addr, int setIndex, int tag, int matchedLineIndex) { // Nothing to do
    log(name(), "read hit on address ", addr, " SET ", setIndex);
    bus->request(addr, false, id, false); // for probe read hit
    wait_for_response(addr, id);

    stats_readhit(id);
    update_aging_bits(setIndex, matchedLineIndex);
}

void Cache::handle_read_miss(uint64_t addr, int setIndex, int tag) {
    int oldest = find_oldest(setIndex);

    if (get_max_oldest(setIndex) == 7) {
        if (cache[setIndex].lines[oldest].state == MODIFIED || cache[setIndex].lines[oldest].state == OWNED) {
            bus->request(addr, true, id, true); // write back
            wait_for_response(addr, id);
            log(name(), "read miss, line written back because MODIFIED or OWNED", addr);
        }
    }


    bus->request(addr, false, id, false); // will attempt to read other caches first and if necessary then memory

    if (wait_for_response(addr, id)) { // went to memory, thus unique
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "read miss, from memory to EXCLUSIVE", addr);
    }
    else { // other caches have it
        // TODO: do i need to send the actual data?
        cache[setIndex].lines[oldest].state = SHARED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "read miss, from other cache to SHARED", addr);
    }

    update_aging_bits(setIndex, oldest);
    stats_readmiss(id);

}

void Cache::handle_probe_write_hit(uint64_t addr, int setIndex, int tag, int i) { //basically invalidate when another cache write hits 
    log(name(), "probe write hit on address ", addr);
    if (cache[setIndex].lines[i].state == EXCLUSIVE) {
        cache[setIndex].lines[i].state = INVALID;
        log(name(), "probe write hit, from EXCLUSIVE to INVALID", addr);
    }
    else if (cache[setIndex].lines[i].state == MODIFIED) {
        cache[setIndex].lines[i].state = INVALID;
        log(name(), "probe write hit, from MODIFIED to INVALID", addr);
    }
    else if (cache[setIndex].lines[i].state == OWNED) {
        cache[setIndex].lines[i].state = INVALID;
        log(name(), "probe write hit, from OWNED to INVALID", addr);
    }
    else if (cache[setIndex].lines[i].state == SHARED) {
        cache[setIndex].lines[i].state = INVALID;
        log(name(), "probe write hit, from SHARED to INVALID", addr);
    }
}

void Cache::handle_probe_read_hit(uint64_t addr, int setIndex, int tag, int i) { //TODO: needs to still check if there's a line to update
    log(name(), "probe read hit on address ", addr);
    if (cache[setIndex].lines[i].state == EXCLUSIVE) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe read hit, from EXCLUSIVE to SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == SHARED) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe read hit, stays in SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == MODIFIED) { //TODO: Only one processor can hold the data in the owned state
        cache[setIndex].lines[i].state = OWNED;
        log(name(), "probe read hit, from OWNED to SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == OWNED) {
        cache[setIndex].lines[i].state = OWNED;
        log(name(), "probe read hit, stays in OWNED", addr);
    }
}

void Cache::handle_eviction(uint64_t addr, int setIndex, int tag, int evictionLineIndex) {
}

