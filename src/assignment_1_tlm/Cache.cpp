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


struct CacheLine {
    uint64_t tag;
    uint32_t data[8]; // 32 byte line
    bool state; // valid/invalid bit
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
            if (cache[setIndex].agingBits[i] > 1) {
                cache[setIndex].agingBits[i] -= 1;
            }
        } else {
            cache[setIndex].agingBits[i] = 8; // set as freshest entry
        }
    }
}

bool Cache::containsZero(uint8_t* agingBits) {
    return std::any_of(agingBits, agingBits + 8, [](int i) { return i == 1; });
}

int Cache::find_oldest(uint64_t setIndex) {
    return std::distance(cache[setIndex].agingBits, std::min_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8));
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_read(uint64_t addr) {
    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    // uint64_t blockOffset = addr & blockOffsetMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);


    bool tagMatch = false;
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
            log(name(), "read hit on address", addr);
            
            tagMatch = true;
            cache[setIndex].lines[i].tag = tag;
            update_aging_bits(setIndex, i);
            stats_readhit(id);
            break;
        } 
    }
    if (!tagMatch) {
        log(name(), "read miss on address", addr);
        stats_readmiss(id);
        int oldest = find_oldest(setIndex);

        log(name(), "read address", addr);

        // bus->read_to_bus(addr);
        bus->snoop(addr, id, 0);
        bus->request(addr, false, id);
        wait_for_response(addr, id); 

        if (oldest == 0) 
        {
            oldest = 1;
        }
        cache[setIndex].lines[oldest].state = 1;
        log(name(), "cacheline valid again: ", addr);
        cache[setIndex].lines[oldest].tag = tag;
        update_aging_bits(setIndex, oldest);
    }
    wait(1);
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

    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
            tagMatch = true;
            cache[setIndex].lines[i].tag = tag;
            update_aging_bits(setIndex, i); 
            stats_writehit(id);

            bus->snoop(addr, id, 1);
            bus->request(addr, true, id); // write-through
            wait_for_response(addr, id); 

            break;
        } 
    }


    if (!tagMatch) {
        log(name(), "write miss on address", addr);
        stats_writemiss(id);
        int oldest = find_oldest(setIndex);

        bus->snoop(addr, id, 1);
        log(name(), "sending bus request", addr);
        bus->request(addr, true, id);

        // wait until bus invokes response_received
        wait_for_response(addr, id); 
        log(name(), "received response back from memory", addr);       

        if (oldest == 0) oldest = 1;
        cache[setIndex].lines[oldest].state = 1;
        log(name(), "cacheline valid again: ", addr); 
        cache[setIndex].lines[oldest].tag = tag;
        update_aging_bits(setIndex, oldest);
    }
    wait(1);
    RET_RESPONSE = false;

    return 0; // indicates succes.
}

int Cache::read_snoop(uint64_t addr, bool isWrite) {
    log(name(), "snoop request on address", addr);
    if (isWrite) {
        has_cacheline(addr);
    } else {
        return 1; // TODO: handle reads as well in the future?
    }
    wait(1);
    return 1;
}

void Cache::wait_for_response(uint64_t addr, int id) {
    log(name(), "waiting for response from memory", addr);
    wait(memory_write_event);
    log(name(), "received response from memory", addr);
}

void Cache::response_received(uint64_t addr, int id) {
    RET_RESPONSE = true;
    // wait(1);
}

bool Cache::has_cacheline(uint64_t addr) {
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
            cache[setIndex].lines[i].state = 0;
            log(name(), "invalidating line at address", addr);
            return true;
        }
    }
    return false;
}


// wipe the data when the class instance is destroyed as a member function
