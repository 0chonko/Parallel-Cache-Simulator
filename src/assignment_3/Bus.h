#ifndef BUS_H
#define BUS_H

#include <iostream>
#include <vector>
#include <queue>
#include <systemc.h>

#include "bus_slave_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "psa.h"
#include "Cache.h"


class Bus : public bus_slave_if, public bus_master_if,  sc_module {
public:    
    sc_port<bus_slave_if> memory; // Bus to memory
    sc_signal<sc_uint<64>> out; // Bus to cache
    std::vector<Cache*> caches;
    sc_in_clk clock;
    struct BusRequest {
        uint64_t addr;
        int id;
        bool isWrite;
        bool busWB;
    };
    //fifo queue of busRequests
    std::queue<BusRequest> requestQueue;
    std::queue<BusRequest> responseQueue;
    sc_event bus_busy;
    sc_event bus_free;

    // (bus_slave_if)
    int read(uint64_t addr, int id) override;
    int write(uint64_t addr, int id) override;
    void request(uint64_t addr, bool isWrite, int id, bool busWB) override;
    int snoop(uint64_t addr, int src_cache, bool isWrite) override;
    bool busy() override;
    BusRequest getNextRequest(); // Declaration of getNextRequest() method
    void pushRequest(BusRequest request);
    void processRequest(BusRequest request);
    void execute();
    void ret_memory_response(uint64_t addr, int id);

    SC_HAS_PROCESS(Bus);

    Bus(sc_module_name name) : sc_module(name) { //TODO: add what else is necessary 
        state = IDLE;
        SC_THREAD(execute);
        sensitive << clock.pos();
    }

    ~Bus() {
        // calculate average of avg_acquisition_time
        sc_time sum = sc_time(0, SC_NS);
        for (auto &time : avg_acquisition_time) {
            sum += time;
        }
        sc_time avg = sum / avg_acquisition_time.size();
        std::cout << "Average acquisition time: " << avg << std::endl;
    }
    
private:
    enum BusState {
            IDLE,
            OCCUPIED
        };

    BusState state;
    std::vector<sc_time> avg_acquisition_time;
    sc_time current_timestamp;
    uint64_t current_addr;
    sc_mutex bus_mutex;
};

#endif // BUS_H
