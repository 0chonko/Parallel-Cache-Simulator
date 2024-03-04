#ifndef BUS_H
#define BUS_H

#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "helpers.h"

class Bus : public bus_slave_if, public sc_module {
public:
    // Input ports for requests from caches and memory
    sc_in<bool> cache_request;
    sc_in<bool> memory_request;

    // Output ports for selected request and snooping messages
    sc_out<bool> selected_request;
    sc_out<bool> snooping_message;

private:
    // Internal variables for request arbitration
    bool cache_request_pending;
    bool memory_request_pending;

public:
    // Constructor
    SC_CTOR(Bus) {
        // Initialize internal variables
        cache_request_pending = false;
        memory_request_pending = false;

        // Register the process for bus operation
        SC_METHOD(busOperation);
        sensitive << cache_request << memory_request;
    }

    // Bus operation process
    void busOperation();

    // Read request (bus_slave_if) 
    virtual int read(uint64_t addr);

    // Write request (bus_slave_if)
    virtual int write(uint64_t addr);

    // Destructor
    virtual ~Bus() {
        // nothing to do here right now.
    }
};

#endif 