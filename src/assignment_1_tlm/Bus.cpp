#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "helpers.h"

class Bus : public bus_slave_if, public sc_module {
    // Input ports for requests from caches and memory
    sc_in<bool> cache_request;
    sc_in<bool> memory_request;

    // Output ports for selected request and snooping messages
    sc_out<bool> selected_request;
    sc_out<bool> snooping_message;

    // Internal variables for request arbitration
    bool cache_request_pending;
    bool memory_request_pending;

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
    void busOperation() {
        // Check if there is a cache request
        if (cache_request.read()) {
            // Set cache request pending
            cache_request_pending = true;
        }

        // Check if there is a memory request
        if (memory_request.read()) {
            // Set memory request pending
            memory_request_pending = true;
        }

        // Check if there is a pending cache request and no pending memory request
        if (cache_request_pending && !memory_request_pending) {
            // Select cache request
            selected_request.write(true);
            snooping_message.write(false);

            // Reset cache request pending
            cache_request_pending = false;
        }

        // Check if there is a pending memory request and no pending cache request
        if (memory_request_pending && !cache_request_pending) {
            // Select memory request
            selected_request.write(true);
            snooping_message.write(false);

            // Reset memory request pending
            memory_request_pending = false;
        }

        // Check if there are both cache and memory requests pending
        if (cache_request_pending && memory_request_pending) {
            // Select cache request and broadcast snooping message
            selected_request.write(true);
            snooping_message.write(true);

            // Reset cache and memory request pending
            cache_request_pending = false;
            memory_request_pending = false;
        }

        // No request selected
        if (!cache_request_pending && !memory_request_pending) {
            selected_request.write(false);
            snooping_message.write(false);
        }
    }

    // Read request (bus_slave_if)
    virtual int read(uint64_t addr) {
        // Check if there is a pending cache request
        if (cache_request_pending) {
            // Return busy
            return BUSY;
        }

        // Return OK
        return OK;
    }
    
    // Write request (bus_slave_if)
    virtual int write(uint64_t addr) {
        // Check if there is a pending cache request
        if (cache_request_pending) {
            // Return busy
            return BUSY;
        }

        // Return OK
        return OK;
    }

    
};
Z