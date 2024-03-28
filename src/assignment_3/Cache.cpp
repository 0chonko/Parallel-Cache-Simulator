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



// // Initialization should use 'new[]' for arrays
bool debug = true;



// dumps the content of the cache
void Cache::dump_cache() {
    using std::cout;
    using std::endl;
    using std::setw;

    for (size_t i = 0; i < SET_COUNT; i++) {
        // print every line in every set in the cache
        for (size_t j = 0; j < 8; j++) {
            size_t index = (i * 8) + j;
            if (index < CACHE_SIZE) {
                if (cache[i].lines[j].state != 0) {
                    cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].state << endl;
                    if (index % 8 == 7) {
                        cout << endl;
                    }
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
                if (cache[setIndex].agingBits[i] < 7) {
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
    log(name(), "cpu_read on address", addr);
    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);


    bool tagMatch = false;

    // READ HIT
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state != INVALID) {
            tagMatch = true;
            handle_read_hit(addr, setIndex, tag, i);
            break;
        }
    }

    // READ MISS
    if (!tagMatch) {
        handle_read_miss(addr, setIndex, tag);
    }
    RET_RESPONSE = false;
    return 0; // Done, return value 0 signals succes.
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_write(uint64_t addr) {

    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);

    bool tagMatch = false;


    // WRITE HIT
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].state != INVALID) {
            if (cache[setIndex].lines[i].tag == tag) {
                tagMatch = true;
                handle_write_hit(addr, setIndex, tag, i);
                break;
            }
        }
    }

    // WRITE MISS
    if (!tagMatch) {
        handle_write_miss(addr, setIndex, tag);
    }

    return 0; // indicates succes.
}

int Cache::read_snoop(uint64_t addr, bool isWrite) {
    return 0;
}


bool Cache::wait_for_response(uint64_t addr, int id) { 
    wait(memory_write_event | status_update_event);
    if (memory_write_event.triggered()) {
        log(name(), "memory_write_event has triggered");
        return true;
    }
    if (status_update_event.triggered()) {
        log(name(), "status_update_event has triggered");
        return false;
    }
   return false;
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

void Cache::handle_write_hit(uint64_t addr, int setIndex, uint64_t tag, int matchedLineIndex) { 
    log(name(), "write hit address ", addr);
    stats_writehit(id);


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

    bus->request(addr, true, id, false, true); // write-through and send probe
    wait_for_response(addr, id);

    update_aging_bits(setIndex, matchedLineIndex);
    wait();

    // if (cache[setIndex].lines[matchedLineIndex].state != INVALID) { // TODO: meant to double check if in the meanwhile stuff has been invalidated
    //     stats_writehit(id);
    //     update_aging_bits(setIndex, matchedLineIndex);

    //  } else {
    //     handle_write_miss(addr, setIndex, tag);
    //  }


}

void Cache::handle_write_miss(uint64_t addr, int setIndex, uint64_t tag) {
    int oldest = find_oldest(setIndex);
    if (get_max_oldest(setIndex) == 7) {
        log(name(), "eviction of ", oldest);
        if (cache[setIndex].lines[oldest].state == MODIFIED || cache[setIndex].lines[oldest].state == OWNED) {
            bus->request(addr, true, id, true, false); // write back
            wait_for_response(addr, id);
            log(name(), "write miss, line written back because MODIFIED or OWNED", addr);
            exit(0);
        }
    }


    bus->request(addr, false, id, false, false); // pull data

    stats_writemiss(id);


    if (wait_for_response(addr, id)) { // if true it went to memory
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        log(name(), "allocate on write pulled data and did not find other copies of requested data. Set to EXCLUSIVE", addr);
        bus->request(addr, true, id, false, true); 
        wait_for_response(addr, id); 
        cache[setIndex].lines[oldest].state = MODIFIED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "write miss, from memory to MODIFIED", addr);
    }
    else { // other caches have it
        cache[setIndex].lines[oldest].state = SHARED;
        log(name(), "allocate on write pulled data and found other copies of requested data. Set to SHARED", addr);
        bus->request(addr, true, id, false, true); // invalidate other copies
        wait_for_response(addr, id);
        cache[setIndex].lines[oldest].state = MODIFIED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "write miss, from other cache to MODIFIED", addr);
    }

    update_aging_bits(setIndex, oldest);
    wait();


}

void Cache::handle_read_hit(uint64_t addr, int setIndex, uint64_t tag, int matchedLineIndex) { // Nothing to do
    stats_readhit(id);

    log(name(), "read hit on address ", addr, " SET ", setIndex);
    bus->request(addr, false, id, false, true); // for probe read hit
    wait_for_response(addr, id);

    update_aging_bits(setIndex, matchedLineIndex);
    wait();
}

void Cache::handle_read_miss(uint64_t addr, int setIndex, uint64_t tag) {
    stats_readmiss(id);

    int oldest = find_oldest(setIndex);

    if (get_max_oldest(setIndex) == 7) {
        log(name(), "eviction of ", oldest);
        if (cache[setIndex].lines[oldest].state == MODIFIED || cache[setIndex].lines[oldest].state == OWNED) {
            bus->request(addr, false, id, true, false); // write back
            wait_for_response(addr, id);
            log(name(), "read miss, line written back because MODIFIED or OWNED", addr);
            cout << "eviction of " << oldest << endl;
            exit(0);
        }
    }


    bus->request(addr, false, id, false, false); // will attempt to read other caches first and if necessary then memory

    if (wait_for_response(addr, id)) { // went to memory, thus unique
        cache[setIndex].lines[oldest].state = EXCLUSIVE;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "read miss, from memory to EXCLUSIVE", addr);
    }
    else { // other caches have it
        cache[setIndex].lines[oldest].state = SHARED;
        cache[setIndex].lines[oldest].tag = tag;
        log(name(), "read miss, from other cache to SHARED", addr);
    }

    update_aging_bits(setIndex, oldest);
    wait();

}

void Cache::handle_probe_write_hit(uint64_t addr, int setIndex, uint64_t tag, int i) { //basically invalidate when another cache write hits 
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
    wait();
}

void Cache::handle_probe_read_hit(uint64_t addr, int setIndex, uint64_t tag, int i) { 
    log(name(), "probe read hit on address ", addr);
    if (cache[setIndex].lines[i].state == EXCLUSIVE) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe read hit, from EXCLUSIVE to SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == SHARED) {
        cache[setIndex].lines[i].state = SHARED;
        log(name(), "probe read hit, stays in SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == MODIFIED) { 
        cache[setIndex].lines[i].state = OWNED;
        log(name(), "probe read hit, from OWNED to SHARED", addr);
    }
    else if (cache[setIndex].lines[i].state == OWNED) {
        cache[setIndex].lines[i].state = OWNED;
        log(name(), "probe read hit, stays in OWNED", addr);
    }
    wait();
}

void Cache::handle_eviction(uint64_t addr, int setIndex, uint64_t tag, int evictionLineIndex) {
}


