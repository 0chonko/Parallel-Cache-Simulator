#include <assert.h>
#include <systemc.h>

#include "Cache.h"
#include "psa.h"

// its an 8-way associative with 32-byte cache lines.
// add the state variables for the cache.

struct CacheLine
{
    uint64_t tag;
    uint32_t data[8]; // 32 byte line
    bool state;
    bool dirty;
};

struct CacheSet
{
    CacheLine lines[8];   // 8-way
    uint8_t agingBits[8]; // 3-bit aging bits for each line, stored as 8-bit integers
};

// signal in
sc_in<bool> Port_CLK;
sc_in<Function> Port_Func;
sc_in<uint64_t> Port_Addr;

// signal out
sc_out<RetCode> Port_Done;

// signal inout
sc_inout_rv<64> Port_Data;

m_data = new uint64_t[CACHE_SIZE];
cache = new CacheSet[SET_COUNT];

// TODO: the cache gets a request from its processor and then forwards it to the bus
// the cache then gets the response from the bus and forwards it to the processor
// requests can be directed to the memory or to other caches

// dumps the content of the cache
void dump_cache()
{
    for (size_t i = 0; i < SET_COUNT; i++)
    {
        // print every line in every set in the cache
        for (size_t j = 0; j < 8; j++)
        {
            for (size_t k = 0; k < 8; k++)
            {
                size_t index = (i * 8 * 8) + (j * 8) + k;
                if (index < CACHE_SIZE)
                {
                    cout << setw(5) << index << ": " << setw(5) << cache[i].lines[j].data[k];
                    if (index % 8 == 7)
                    {
                        cout << endl;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
}

    uint64_t *m_data;
    CacheSet *cache;

    // Define the number of bits for the set index and block offset
    const int SET_INDEX_BITS = 7;
    const int BLOCK_OFFSET_BITS = 5;
    const int TAG_BITS = 64 - SET_INDEX_BITS - BLOCK_OFFSET_BITS;
    // Calculate the masks for the set index and block offset
    uint64_t setIndexMask = (1ULL << SET_INDEX_BITS) - 1;
    uint64_t blockOffsetMask = (1ULL << BLOCK_OFFSET_BITS) - 1;
    uint64_t setTagMask = ((1ULL << SET_INDEX_BITS) - 1) << BLOCK_OFFSET_BITS;

    uint64_t tagMask = (1ULL << TAG_BITS) - 1;
    // TODO: check for tag

    void update_aging_bits(uint64_t setIndex, uint64_t blockOffset) // active aging bits are 1-8, empty line has 0
    {
        for (int i = 0; i < 8; i++)
        {
            if (i != blockOffset)
            {
                if (cache[setIndex].agingBits[i] > 1)
                {
                    cache[setIndex].agingBits[i] -= 1;
                }
            }
            else
            {
                cache[setIndex].agingBits[i] = 8; // set as freshest entry
            }
        }
    }

bool containsZero(uint8_t * agingBits)
{
    return std::any_of(agingBits, agingBits + 8, [](int i)
                        { return i == 0; });
}

// function find oldest which searches for oldest line in the set, which has the smallest aging bit in one line (using std::min_element) and should return the index of that bit
int find_oldest(uint64_t setIndex)
{
    return std::distance(cache[setIndex].agingBits, std::min_element(cache[setIndex].agingBits, cache[setIndex].agingBits + 8));
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_read(uint64_t addr) {
    uint64_t data = 0;
    uint32_t ret_data = 0;
    // cout << sc_time_stamp() << "your port data received is " << Port_Addr.read() << endl;

    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t blockOffset = addr & blockOffsetMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);

    
    // int hit = rand() % 2; // cache not modeled.
    // int dirty = rand() % 2;

    bool tagMatch = false;
    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
            if(debug) cout << sc_time_stamp() << ": MEM received read hit" << endl;
            stats_readhit(0);
            tagMatch = true;
            ret_data = cache[setIndex].lines[i].data[blockOffset];
            cache[setIndex].lines[i].tag = tag;
            update_aging_bits(setIndex, i);
            break;
        }
        wait(1);
    }
    if (!tagMatch) {
        if(debug) cout << sc_time_stamp() << ": MEM received read miss" << endl;
        stats_readmiss(0);
        int oldest = find_oldest(setIndex);
        if (cache[setIndex].lines[oldest].dirty)
        {   
            if(debug) cout << sc_time_stamp() << ": MEM received read miss with dirty, writing back" << endl;
            cache[setIndex].lines[oldest].dirty = 0;
        }
        wait(100);
        if (oldest == 0) 
        {
            if(debug) cout << sc_time_stamp() << ": MEM received read on a empty line" << endl;
            oldest = 1;
        }
        cache[setIndex].lines[oldest].state = 1;
        cache[setIndex].lines[oldest].tag = tag;
        ret_data = cache[setIndex].lines[oldest].data[blockOffset];
        update_aging_bits(setIndex, oldest);
    }

    if (hit) { // Hit!
        log(name(), "read hit on address", addr);
        stats_readhit(id);
        return 0; // succes
    }

    log(name(), "read miss on address", addr);
    stats_readmiss(id);
    if (dirty) {
        // Write back
        log(name(), "replacement write back");
        memory->write(128); // cache not modeled so address unknown
    }

    // Cache miss, read cache line from memory.
    log(name(), "read address", addr);
    memory->read(addr);
    return 0; // Done, return value 0 signals succes.
}

/* cpu_cache_if interface method
 * Called by CPU.
 */
int Cache::cpu_write(uint64_t addr) {
    uint64_t data = 0;
    uint32_t ret_data = 0;
    // cout << sc_time_stamp() << "your port data received is " << Port_Addr.read() << endl;

    // Extract the set index and block offset
    uint64_t setIndex = (addr >> BLOCK_OFFSET_BITS) & setIndexMask;
    uint64_t blockOffset = addr & blockOffsetMask;
    uint64_t tag = addr >> (SET_INDEX_BITS + BLOCK_OFFSET_BITS);

    bool tagMatch = false;

    for (uint64_t i = 0; i < SET_SIZE; i++) {
        if (cache[setIndex].lines[i].tag == tag && cache[setIndex].lines[i].state == 1) {
            if(debug) cout << sc_time_stamp() << ": MEM received write hit" << endl;
            stats_writehit(0);
            tagMatch = true;
            // cache[setIndex].lines[i].data[blockOffset] = data;
            cache[setIndex].lines[i].tag = tag;
            update_aging_bits(setIndex, i);
            break;
        }
    }
    if (!tagMatch) {
        // if(debug) cout << sc_time_stamp() << ": MEM received write miss" << endl;
        log(name(), "write miss on address", addr);
        stats_writemiss(id);
        int oldest = find_oldest(setIndex);
        wait(100);
        if (cache[setIndex].lines[oldest].dirty) cache[setIndex].lines[oldest].dirty = 0;
        if (oldest == 0) oldest = 1;
        cache[setIndex].lines[oldest].state = 1;
        cache[setIndex].lines[oldest].tag = tag;
        // cache[setIndex].lines[oldest].data[blockOffset] = data;
        cache[setIndex].lines[oldest].dirty = 1;
        update_aging_bits(setIndex, oldest);
    }
    // int hit = rand() % 2; // cache not modeled.
    // int dirty = rand() % 2;

    if (hit) { // Hit!
        log(name(), "write hit on address", addr);
        stats_writehit(id);
        // write back cache, so don't write through to memory.
        return 0;
    }

    log(name(), "write miss on address", addr);
    stats_writemiss(id);
    if (dirty) { // cache line is dirty.
        // Write back cache line to memory
        log(name(), "replacement write back");
        memory->write(128); // cache not modeled so address unknown
    }
    // Read complete cache line from memory.
    log(name(), "read address", addr);
    memory->read(addr);

    return 0; // indicates succes.
}

// wipe the data when the class instance is destroyed
~Cache() {
    delete[] m_data;
    delete[] cache;
}
///////////////////////////////////////////////////////////////////////////




if (f == FUNC_READ)
{
    // read at addr and return ret code
    Port_Data.write((addr < CACHE_SIZE) ? ret_data : 0);
    Port_Done.write(RET_READ_DONE);
    wait();
    Port_Data.write(float_64_bit_wire); // string with 64 "Z"'s
}
else // writing
{
    Port_Done.write(RET_WRITE_DONE);
}

// wait(100);
}
}
};

